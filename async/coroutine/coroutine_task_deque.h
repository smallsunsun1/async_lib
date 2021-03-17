#ifndef ASYNC_COROUTINE_COROUTINE_TASK_DEQUE_
#define ASYNC_COROUTINE_COROUTINE_TASK_DEQUE_

#include <array>
#include <atomic>
#include <cassert>
#include <limits>
#include <mutex>
#include <vector>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace sss {
namespace async {
class ScheduleOperation;
namespace internal {

class CoroutineTaskDeque {
 public:
  static constexpr unsigned kCapacity = 1024;

  static_assert((kCapacity > 2) && (kCapacity <= (64u << 10u)), "CoroutineTaskDeque capacity must be in [4, 65536] range");
  static_assert((kCapacity & (kCapacity - 1)) == 0, "CoroutineTaskDeque capacity must be a power of two for fast masking");

  CoroutineTaskDeque() : mFront(0), mBack(0) {
    for (unsigned i = 0; i < kCapacity; i++) {
      mArray[i].state.store(kEmpty, std::memory_order_relaxed);
    }
  }
  CoroutineTaskDeque(const CoroutineTaskDeque&) = delete;
  void operator=(const CoroutineTaskDeque&) = delete;

  ~CoroutineTaskDeque() { assert(Size() == 0); }

  // Size returns current queue size.
  // Can be called by any thread at any time.
  unsigned Size() const { return SizeOrNotEmpty<true>(); }

  // Empty tests whether container is empty.
  // Can be called by any thread at any time.
  bool Empty() const { return SizeOrNotEmpty<false>() == 0; }

  absl::optional<ScheduleOperation*> PushFront(ScheduleOperation* task);
  absl::optional<ScheduleOperation*> PopFront();
  absl::optional<ScheduleOperation*> PushBack(ScheduleOperation* task);
  absl::optional<ScheduleOperation*> PopBack();
  unsigned PopBackHalf(std::vector<ScheduleOperation*>* result);

  // Delete all the elements from the queue.
  void Flush() {
    while (!Empty()) {
      absl::optional<ScheduleOperation*> task = PopFront();
      assert(task.has_value());
    }
  }

 private:
  // Constant used to move front/back pointers in Push/Pop functions above.
  static constexpr unsigned kIncrement = (kCapacity << 1u) + 1;

  static constexpr unsigned kMask = kCapacity - 1;
  static constexpr unsigned kMask2 = (kCapacity << 1u) - 1;

  std::mutex mMutex;

  enum : uint8_t { kEmpty = 0, kBusy = 1, kReady = 2 };
  struct Elem {
    std::atomic<uint8_t> state;
    ScheduleOperation* task;
  };

  // Low log(kCapacity) + 1 bits in mFront and mBack contain rolling index of
  // front/back, respectively. The remaining bits contain modification counters
  // that are incremented on Push operations. This allows us to
  //
  // (1) Distinguish between empty and full conditions (if we would use
  //     log(kCapacity) bits for position, these conditions would be
  //     indistinguishable)
  //
  // (2) Obtain consistent snapshot of mFront/mBack for Size operation using the
  //     modification counters.

  // Align to 128 byte boundary to prevent false sharing between front and back
  // pointers, they are accessed from different threads.
  alignas(128) std::atomic<unsigned> mFront;
  alignas(128) std::atomic<unsigned> mBack;

  std::array<Elem, kCapacity> mArray;
  template <bool need_size_estimate>
  unsigned SizeOrNotEmpty() const {
    // Emptiness plays critical role in thread pool blocking. So we go to great
    // effort to not produce false positives (claim non-empty queue as empty).
    unsigned front = mFront.load(std::memory_order_acquire);
    for (;;) {
      // Capture a consistent snapshot of front/tail.
      unsigned back = mBack.load(std::memory_order_acquire);
      unsigned front1 = mFront.load(std::memory_order_relaxed);
      if (front != front1) {
        front = front1;
        std::atomic_thread_fence(std::memory_order_acquire);
        continue;
      }
      if (need_size_estimate) {
        return CalculateSize(front, back);
      } else {
        // This value will be 0 if the queue is empty, and undefined otherwise.
        unsigned maybe_zero = ((front ^ back) & kMask2);
        // Queue size estimate must agree with maybe zero check on the queue
        // empty/non-empty state.
        assert((CalculateSize(front, back) == 0) == (maybe_zero == 0));
        return maybe_zero;
      }
    }
  }
  unsigned CalculateSize(unsigned front, unsigned back) const;
};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_COROUTINE_TASK_DEQUE_ */
