#ifndef ASYNC_CONCURRENT_TASK_PRIORITY_QUEUE_
#define ASYNC_CONCURRENT_TASK_PRIORITY_QUEUE_

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <list>
#include <mutex>
#include <optional>
#include <queue>

#include "async/support/task_function.h"

namespace sss {
namespace async {

enum class TaskPriority : int8_t {
  kCritical = 0,
  kHigh = 1,
  kDefault = 2,
  kLow = 3
};

class TaskPriorityDeque {
  static constexpr uint64_t kCounterBits = 10;  // capacity = 1024

  static constexpr int kNumTaskPriorities = 4;

  static constexpr std::array<TaskPriority, kNumTaskPriorities>
      kTaskPriorities = {
          TaskPriority::kCritical,
          TaskPriority::kHigh,
          TaskPriority::kDefault,
          TaskPriority::kLow,
  };

  static_assert(static_cast<int>(kTaskPriorities[0]) == 0,
                "Unexpected TaskPriority value");
  static_assert(static_cast<int>(kTaskPriorities[1]) == 1,
                "Unexpected TaskPriority value");
  static_assert(static_cast<int>(kTaskPriorities[2]) == 2,
                "Unexpected TaskPriority value");
  static_assert(static_cast<int>(kTaskPriorities[3]) == 3,
                "Unexpected TaskPriority value");

 public:
  static constexpr uint64_t kCapacity = (1ull << kCounterBits);

  static_assert((kCapacity > 2) && (kCapacity <= (1u << 10u)),
                "TaskPriorityDeque capacity must be in [4, 1024] range");
  static_assert(
      (kCapacity & (kCapacity - 1)) == 0,
      "TaskPriorityDeque capacity must be a power of two for fast masking");

  TaskPriorityDeque() : front_(0), back_(0) {
    for (TaskPriority priority : kTaskPriorities) {
      for (unsigned i = 0; i < kCapacity; i++) {
        elem(priority, i)->state.store(kEmpty, std::memory_order_relaxed);
      }
    }
  }
  TaskPriorityDeque(const TaskPriorityDeque &) = delete;
  TaskPriorityDeque &operator=(const TaskPriorityDeque &) = delete;

  ~TaskPriorityDeque() { assert(Size() == 0); }

  // PushFront() inserts task at the beginning of the queue for the specified
  // priority.
  //
  // If the queue is full, returns passed in task wrapped in optional, otherwise
  // returns empty optional.
  std::optional<TaskFunction> PushFront(TaskFunction task,
                                        TaskPriority priority) {
    assert(static_cast<int>(priority) < kNumTaskPriorities);

    PointerState front(front_.load(std::memory_order_relaxed));
    uint64_t index = front.Index(priority);

    Elem *e = elem(priority, index);
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (s != kEmpty || !e->state.compare_exchange_strong(
                           s, kBusy, std::memory_order_acquire)) {
      return std::optional<TaskFunction>(std::move(task));
    }

    front_.store(front.Inc(priority), std::memory_order_relaxed);
    e->task = std::move(task);
    e->state.store(kReady, std::memory_order_release);
    return std::nullopt;
  }

  std::optional<TaskFunction> PushFront(TaskFunction task) {
    return PushFront(std::move(task), TaskPriority::kDefault);
  }

  // PopFront() iterates through all queues in their priority order and removes
  // and returns the first element from the first non-empty queue.
  //
  // If all queues are empty returns empty optional.
  std::optional<TaskFunction> PopFront() {
    PointerState front(front_.load(std::memory_order_relaxed));

    for (TaskPriority priority : kTaskPriorities) {
      uint64_t index = front.IndexExt(priority);

      Elem *e = elem(priority, (index - 1) & kIndexMask);
      uint8_t s = e->state.load(std::memory_order_relaxed);

      if (s != kReady) continue;
      if (!e->state.compare_exchange_strong(s, kBusy,
                                            std::memory_order_acquire)) {
        return std::nullopt;
      }

      TaskFunction task = std::move(e->task);
      e->state.store(kEmpty, std::memory_order_release);
      front_.store(front.WithIndexExt((index - 1) & kIndexMaskExt, priority),
                   std::memory_order_relaxed);

      return std::optional<TaskFunction>(std::move(task));
    }

    // No tasks found at any priority level.
    return std::nullopt;
  }

  // PushBack() inserts task `w` at the end of the queue for the specified
  // priority.
  //
  // If all queues are full, returns passed in task wrapped in optional,
  // otherwise returns empty optional.
  std::optional<TaskFunction> PushBack(TaskFunction task,
                                       TaskPriority priority) {
    assert(static_cast<int>(priority) < kNumTaskPriorities);

    std::lock_guard<std::mutex> lock(mutex_);
    PointerState back(back_.load(std::memory_order_relaxed));
    uint64_t index = back.IndexExt(priority);

    Elem *e = elem(priority, (index - 1) & kIndexMask);
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (s != kEmpty || !e->state.compare_exchange_strong(
                           s, kBusy, std::memory_order_acquire)) {
      return std::optional<TaskFunction>(std::move(task));
    }

    back_.store(back.WithIndexExt((index - 1) & kIndexMaskExt, priority),
                std::memory_order_relaxed);
    e->task = std::move(task);
    e->state.store(kReady, std::memory_order_release);
    return std::nullopt;
  }

  std::optional<TaskFunction> PushBack(TaskFunction task) {
    return PushBack(std::move(task), TaskPriority::kDefault);
  }

  // PopBack() iterates through all queues in their priority order and removes
  // and returns the last elements from the first non-empty queue.
  //
  // If all queues are empty returns empty optional.
  std::optional<TaskFunction> PopBack() {
    if (Empty()) return std::nullopt;

    std::lock_guard<std::mutex> lock(mutex_);
    PointerState back(back_.load(std::memory_order_relaxed));

    for (TaskPriority priority : kTaskPriorities) {
      Elem *e = elem(priority, back.Index(priority));
      uint8_t s = e->state.load(std::memory_order_relaxed);

      if (s != kReady) continue;
      if (!e->state.compare_exchange_strong(s, kBusy,
                                            std::memory_order_acquire)) {
        return std::nullopt;
      }

      TaskFunction task = std::move(e->task);
      e->state.store(kEmpty, std::memory_order_release);
      back_.store(back.Inc(priority), std::memory_order_relaxed);

      return std::optional<TaskFunction>(std::move(task));
    }

    // No tasks found at any priority level.
    return std::nullopt;
  }

  // Size returns current queue size (sum of sizes for all priority levels).
  // Can be called by any thread at any time.
  uint64_t Size() const { return SizeOrNotEmpty<true>(); }

  // Empty tests whether container is empty.
  // Can be called by any thread at any time.
  bool Empty() const { return SizeOrNotEmpty<false>() == 0; }

  // Delete all the elements from the queue.
  void Flush() {
    while (!Empty()) {
      std::optional<TaskFunction> task = PopFront();
      assert(task.has_value());
    }
  }

 private:
  // We use log2(kCapacity) + 1 bits to store rolling index of front/back
  // elements for all priority levels.
  //
  // We need +1 bit in the index to distinguish between empty and full
  // conditions (if we would use log2(kCapacity) bits for position, these
  // conditions would be indistinguishable).

  // Mask index in the lowest bits.
  static constexpr uint64_t kIndexMask = (1ull << kCounterBits) - 1;

  // Mask index + overflow bit in the lower bits.
  static constexpr uint64_t kIndexMaskExt = (1ull << (kCounterBits + 1)) - 1;

  // Offsets for indices in front/back for different priority levels.
  static constexpr std::array<uint64_t, kNumTaskPriorities> kIndexShift = {
      (kCounterBits + 1) * 0,  // Critical
      (kCounterBits + 1) * 1,  // High
      (kCounterBits + 1) * 2,  // Default
      (kCounterBits + 1) * 3,  // Low
  };

  // Masks for extracting index values from front/back.
  static constexpr std::array<uint64_t, kNumTaskPriorities> kIndexMasks = {
      kIndexMask << kIndexShift[0],  // Critical
      kIndexMask << kIndexShift[1],  // High
      kIndexMask << kIndexShift[2],  // Default
      kIndexMask << kIndexShift[3],  // Low
  };

  // Masks for extracting index plus overflow bit.
  static constexpr std::array<uint64_t, kNumTaskPriorities> kIndexMasksExt = {
      kIndexMaskExt << kIndexShift[0],  // Critical
      kIndexMaskExt << kIndexShift[1],  // High
      kIndexMaskExt << kIndexShift[2],  // Default
      kIndexMaskExt << kIndexShift[3],  // Low
  };

  // Masks for extracting two complementary indices.
  static constexpr std::array<uint64_t, kNumTaskPriorities> kIndexMaskCompl = {
      kIndexMasksExt[1] | kIndexMasksExt[2] | kIndexMasksExt[3],  // 0 vs 1,2,3
      kIndexMasksExt[0] | kIndexMasksExt[2] | kIndexMasksExt[3],  // 1 vs 0,2,3
      kIndexMasksExt[0] | kIndexMasksExt[1] | kIndexMasksExt[3],  // 2 vs 0,1,3
      kIndexMasksExt[0] | kIndexMasksExt[1] | kIndexMasksExt[2],  // 3 vs 0,1,2
  };

  // Mask for extracting all indices at once.
  static constexpr uint64_t kIndicesBits =
      (kCounterBits + 1) * kNumTaskPriorities;
  static constexpr uint64_t kIndicesMask = (1ull << kIndicesBits) - 1;

  // The remaining bits contain modification counter that is incremented on Push
  // operations. This allows us to obtain consistent snapshot of front/back for
  // Size operation using the modification counters.
  static constexpr uint64_t kModificationCounterBits = 64 - kIndicesBits;
  static_assert(kModificationCounterBits >= 20,
                "Not enough bits for the modification counter");

  // Mask for extracting modification counter from lower bits.
  static constexpr uint64_t kModificationCounterMask =
      (1ull << kModificationCounterBits) - 1;

  // PointerState helps to read and modify the indices state for different
  // priority levels encoded in the front and back atomics.
  struct PointerState {
    explicit PointerState(uint64_t state) : state(state) {}

    uint64_t Index(TaskPriority priority) const {
      const int idx = static_cast<int>(priority);
      return (state & kIndexMasks[idx]) >> kIndexShift[idx];
    }

    template <TaskPriority priority>
    uint64_t Index() const {
      return Index(priority);
    }

    uint64_t IndexExt(TaskPriority priority) const {
      const int idx = static_cast<int>(priority);
      return (state & kIndexMasksExt[idx]) >> kIndexShift[idx];
    }

    template <TaskPriority priority>
    uint64_t IndexExt() const {
      return IndexExt(priority);
    }

    uint64_t WithIndexExt(uint64_t index, TaskPriority priority) const {
      const int idx = static_cast<int>(priority);
      assert((index & ~kIndexMaskExt) == 0);
      return (index << kIndexShift[idx]) | (state & ~kIndexMasksExt[idx]);
    }

    uint64_t Inc(TaskPriority priority) const {
      const int idx = static_cast<int>(priority);

      // Index for the given priority.
      const uint64_t index = (state & kIndexMasksExt[idx]) >> kIndexShift[idx];
      // Two other indices.
      const uint64_t other_indices = state & kIndexMaskCompl[idx];
      // Modification counter.
      const uint64_t counter = (state & ~kIndicesBits) >> kIndicesBits;

      // Stitch all pieces back together.
      const uint64_t inc =
          // increment index pointer
          (((index + 1) & kIndexMaskExt) << kIndexShift[idx]) |
          // increment modification counter
          (((counter + 1) & kModificationCounterMask) << kIndicesBits) |
          // copy two other indices
          other_indices;

      CheckState(priority, inc);

      return inc;
    }

    // Check correctness of the pointer state after index increment.
    void CheckState(TaskPriority priority, uint64_t inc) const {
      const int idx = static_cast<int>(priority);

      // Parse previous and new index value.
      const uint64_t prev_idx = (state & kIndexMasks[idx]) >> kIndexShift[idx];
      const uint64_t new_idx = (inc & kIndexMasks[idx]) >> kIndexShift[idx];
      (void)prev_idx;
      (void)new_idx;

      // Index must be incremented by one ...
      assert((prev_idx == (kCapacity - 1)) || (new_idx - prev_idx) == 1);
      // ... or it must wrap around to zero.
      assert((prev_idx != (kCapacity - 1)) || new_idx == 0);

      // Parse previous and new modification counter value.
      const uint64_t prev_cnt = (state & ~kIndicesMask) >> kIndicesBits;
      const uint64_t new_cnt = (inc & ~kIndicesMask) >> kIndicesBits;
      (void)prev_cnt;
      (void)new_cnt;

      // Capacity of the modification counter.
      static constexpr uint64_t kCntCapacity = 1ull << kModificationCounterBits;
      (void)kCntCapacity;

      // Modification counter must be incremented by one ...
      assert((prev_cnt == (kCntCapacity - 1)) || (new_cnt - prev_cnt) == 1);
      // ... or it must wrap around to zero.
      assert((prev_cnt != (kCntCapacity - 1)) || new_cnt == 0);
    }

    uint64_t state;
  };

  enum : uint8_t {
    kEmpty = 0,
    kBusy = 1,
    kReady = 2,
  };

  struct Elem {
    std::atomic<uint8_t> state;
    TaskFunction task;
  };

  std::mutex mutex_;

  // Front and back pointers store 3 rolling indices for queues of different
  // priority levels, and a modification counter in the remaining bits.
  //
  // See schema for the front and back layouts above.

  // Align to 128 byte boundary to prevent false sharing between front and back
  // pointers, they are accessed from different threads.
  alignas(128) std::atomic<uint64_t> front_;
  alignas(128) std::atomic<uint64_t> back_;

  // Align to 128 bytes boundary to prevent sharing first `state` of the
  // priority level 0 with back, they are accessed from different threads.
  alignas(128) std::array<Elem, kCapacity * kTaskPriorities.size()> array_;

  // Get a pointer to the task queue storage element for the given priority and
  // offset.
  Elem *elem(TaskPriority priority, size_t offset) {
    assert(static_cast<int>(priority) < kNumTaskPriorities);
    assert(offset < kCapacity);
    return &array_[static_cast<int>(priority) * kCapacity + offset];
  }

  // SizeOrNotEmpty() returns current queue size; if `need_size_estimate` is
  // false, only whether the size is 0 is guaranteed to be correct. Can be
  // called by any thread at any time.
  template <bool need_size_estimate>
  uint64_t SizeOrNotEmpty() const {
    // Emptiness plays critical role in thread pool blocking. So we go to great
    // effort to not produce false positives (claim non-empty queue as empty).
    uint64_t front = front_.load(std::memory_order_acquire);
    for (;;) {
      // Capture a consistent snapshot of front/tail.
      uint64_t back = back_.load(std::memory_order_acquire);
      uint64_t front1 = front_.load(std::memory_order_relaxed);
      if (front != front1) {
        front = front1;
        std::atomic_thread_fence(std::memory_order_acquire);
        continue;
      }
      if (need_size_estimate) {
        return CalculateSize(front, back);
      } else {
        // This value will be 0 if all deques are empty, and undefined otherwise
        uint64_t maybe_zero = ((front ^ back) & kIndicesMask);
        // Queue size estimate must agree with maybe zero check on the queue
        // empty/non-empty state.
        assert((CalculateSize(front, back) == 0) == (maybe_zero == 0));
        return maybe_zero;
      }
    }
  }

  unsigned CalculateSize(uint64_t front, uint64_t back) const {
    return CalculateSize(PointerState(front), PointerState(back));
  }

  unsigned CalculateSize(PointerState front, PointerState back) const {
    auto queue_size = [&](TaskPriority priority) -> int {
      return front.IndexExt(priority) - back.IndexExt(priority);
    };

    int size0 = queue_size(TaskPriority::kCritical);
    int size1 = queue_size(TaskPriority::kHigh);
    int size2 = queue_size(TaskPriority::kDefault);
    int size3 = queue_size(TaskPriority::kLow);

    // Fix overflow.
    if (size0 < 0) size0 += 2 * kCapacity;
    if (size1 < 0) size1 += 2 * kCapacity;
    if (size2 < 0) size2 += 2 * kCapacity;
    if (size3 < 0) size3 += 2 * kCapacity;

    // Order of modification in push/pop is crafted to make the queue look
    // larger than it is during concurrent modifications. E.g. push can
    // increment size before the corresponding pop has decremented it.
    // So the computed size can be up to kCapacity + 1, fix it.
    assert(size0 <= static_cast<int>(kCapacity) + 1);
    assert(size1 <= static_cast<int>(kCapacity) + 1);
    assert(size2 <= static_cast<int>(kCapacity) + 1);
    assert(size3 <= static_cast<int>(kCapacity) + 1);
    if (size0 > static_cast<int>(kCapacity)) size0 = kCapacity;
    if (size1 > static_cast<int>(kCapacity)) size1 = kCapacity;
    if (size2 > static_cast<int>(kCapacity)) size2 = kCapacity;
    if (size3 > static_cast<int>(kCapacity)) size3 = kCapacity;

    int size = size0 + size1 + size2 + size3;
    assert(static_cast<uint32_t>(size) <= kNumTaskPriorities * kCapacity);

    return size;
  }
};

// 用于非定长Task场景中
class TaskPriorityLockDeque {
 public:
  TaskPriorityLockDeque() = default;
  TaskPriorityLockDeque(const TaskPriorityLockDeque &) = delete;
  TaskPriorityLockDeque &operator=(const TaskPriorityLockDeque &) = delete;
  // 这里实际上可以返回void，为了保持接口一致，返回std::nullopt
  std::optional<TaskFunction> PushFront(TaskFunction task,
                                        TaskPriority priority) {
    std::lock_guard<std::mutex> lk(mu_);
    if (priority == TaskPriority::kCritical) {
      critical_priority_queue_.push_front(std::move(task));
    } else if (priority == TaskPriority::kHigh) {
      high_priority_queue_.push_front(std::move(task));
    } else if (priority == TaskPriority::kDefault) {
      default_priority_queue_.push_front(std::move(task));
    } else {
      low_priority_queue_.push_front(std::move(task));
    }
    elems_ += 1;
    return std::nullopt;
  }
  std::optional<TaskFunction> PushFront(TaskFunction task) {
    return PushFront(std::move(task), TaskPriority::kDefault);
  }
  // 这里实际上可以返回void，为了保持接口一致，返回std::nullopt
  std::optional<TaskFunction> PushBack(TaskFunction task,
                                       TaskPriority priority) {
    std::lock_guard<std::mutex> lk(mu_);
    if (priority == TaskPriority::kCritical) {
      critical_priority_queue_.push_back(std::move(task));
    } else if (priority == TaskPriority::kHigh) {
      high_priority_queue_.push_back(std::move(task));
    } else if (priority == TaskPriority::kDefault) {
      default_priority_queue_.push_back(std::move(task));
    } else {
      low_priority_queue_.push_back(std::move(task));
    }
    elems_ += 1;
    return std::nullopt;
  }
  std::optional<TaskFunction> PushBack(TaskFunction task) {
    return PushBack(std::move(task), TaskPriority::kDefault);
  }
  std::optional<TaskFunction> PopFront() {
    std::lock_guard<std::mutex> lk(mu_);
    if (elems_ == 0) return std::nullopt;
    if (!critical_priority_queue_.empty()) {
      auto res = std::move(critical_priority_queue_.front());
      critical_priority_queue_.pop_front();
      return res;
    }
    if (!high_priority_queue_.empty()) {
      auto res = std::move(high_priority_queue_.front());
      high_priority_queue_.pop_front();
      return res;
    }
    if (!default_priority_queue_.empty()) {
      auto res = std::move(default_priority_queue_.front());
      default_priority_queue_.pop_front();
      return res;
    }
    if (!low_priority_queue_.empty()) {
      auto res = std::move(low_priority_queue_.front());
      low_priority_queue_.pop_front();
      return res;
    }
    elems_ -= 1;
  }
  std::optional<TaskFunction> PopBack() {
    std::lock_guard<std::mutex> lk(mu_);
    if (elems_ == 0) return std::nullopt;
    if (!critical_priority_queue_.empty()) {
      auto res = std::move(critical_priority_queue_.back());
      critical_priority_queue_.pop_back();
      return res;
    }
    if (!high_priority_queue_.empty()) {
      auto res = std::move(high_priority_queue_.back());
      high_priority_queue_.pop_back();
      return res;
    }
    if (!default_priority_queue_.empty()) {
      auto res = std::move(default_priority_queue_.back());
      default_priority_queue_.pop_back();
      return res;
    }
    if (!low_priority_queue_.empty()) {
      auto res = std::move(low_priority_queue_.back());
      low_priority_queue_.pop_back();
      return res;
    }
    elems_ -= 1;
  }
  bool Empty() const {
    std::lock_guard<std::mutex> lock(mu_);
    return elems_ == 0;
  }
  size_t Size() const {
    std::lock_guard<std::mutex> lock(mu_);
    return elems_;
  }
  void Flush() {
    while (!Empty()) {
      std::optional<TaskFunction> task = PopFront();
      assert(task.has_value());
    }
  }

 private:
  mutable std::mutex mu_;
  std::list<TaskFunction> critical_priority_queue_;
  std::list<TaskFunction> high_priority_queue_;
  std::list<TaskFunction> default_priority_queue_;
  std::list<TaskFunction> low_priority_queue_;
  size_t elems_;
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_CONCURRENT_TASK_PRIORITY_QUEUE_ */
