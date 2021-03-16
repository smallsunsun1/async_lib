#ifndef ASYNC_COROUTINE_COROUTINE_TASK_QUEUE_
#define ASYNC_COROUTINE_COROUTINE_TASK_QUEUE_

#include <array>
#include <atomic>
#include <cassert>
#include <limits>
#include <mutex>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ficus {
namespace async {

class ScheduleOperation;

namespace internal {

class CoroutineTaskQueue {
 public:
  static const unsigned kCapacity = 1024;
  static_assert((kCapacity > 2) && (kCapacity <= (64u << 10u)), "CoroutineTaskQueue capacity must be in [4, 65536] range");
  static_assert((kCapacity & (kCapacity - 1)) == 0, "CoroutineTaskQueue capacity must be a power of two for fast masking");
  CoroutineTaskQueue() : mFront(0), mBack(0) {
    for (unsigned i = 0; i < kCapacity; ++i) mArray[i].state.store(i);
  }
  CoroutineTaskQueue(const CoroutineTaskQueue&) = delete;
  CoroutineTaskQueue& operator=(const CoroutineTaskQueue&) = delete;
  absl::optional<ScheduleOperation*> PushFront(ScheduleOperation* task) {
    unsigned front = mFront.load(std::memory_order_relaxed);
    Elem* e;
    for (;;) {
      e = &mArray[front & kMask];
      unsigned state = e->state.load(std::memory_order_acquire);
      int64_t diff = static_cast<int64_t>(state) - static_cast<int64_t>(front);
      // Try to acquire an ownership of element at `front`.
      if (diff == 0 && mFront.compare_exchange_strong(front, front + 1, std::memory_order_relaxed)) {
        break;
      }

      // We wrapped around the queue, and have no space in the queue.
      if (diff < 0) return {task};

      // Another thread acquired element at `front` index.
      if (diff > 0) front = mFront.load(std::memory_order_relaxed);
    }
    e->task = task;
    e->state.store(front + 1, std::memory_order_release);
    return absl::nullopt;
  }
  absl::optional<ScheduleOperation*> PopBack() {
    unsigned back = mBack.load(std::memory_order_relaxed);
    Elem* e;

    for (;;) {
      e = &mArray[back & kMask];
      unsigned state = e->state.load(std::memory_order_acquire);
      int64_t diff = static_cast<int64_t>(state) - static_cast<int64_t>(back + 1);

      // Element at `back` is ready, try to acquire its ownership.
      if (diff == 0 && mBack.compare_exchange_strong(back, back + 1, std::memory_order_relaxed)) {
        break;
      }

      // We've reached an empty element.
      if (diff < 0) return absl::nullopt;

      // Another thread popped a task from element at 'back'.
      if (diff > 0) back = mBack.load(std::memory_order_relaxed);
    }

    ScheduleOperation* task = e->task;
    e->state.store(back + kCapacity, std::memory_order_release);
    return {task};
  }
  unsigned Size() const {
    unsigned front = mFront.load(std::memory_order_acquire);
    for (;;) {
      // Capture a consistent snapshot of front/back.
      unsigned back = mBack.load(std::memory_order_acquire);
      unsigned front1 = mFront.load(std::memory_order_relaxed);
      if (front != front1) {
        front = front1;
        std::atomic_thread_fence(std::memory_order_acquire);
        continue;
      }

      return std::min(kCapacity, front - back);
    }
  }
  bool Empty() const { return Size() == 0; }
  void Flush() {
    while (!Empty()) {
      absl::optional<ScheduleOperation*> task = PopBack();
      assert(task.has_value());
    }
  }

 private:
  static constexpr unsigned kMask = kCapacity - 1;
  struct Elem {
    std::atomic<unsigned> state;
    ScheduleOperation* task;
  };
  // Align to 128 byte boundary to prevent false sharing between front and back
  // pointers, they are accessed from different threads.
  alignas(128) std::atomic<unsigned> mFront;
  alignas(128) std::atomic<unsigned> mBack;

  std::array<Elem, kCapacity> mArray;
};

}  // namespace internal
}  // namespace async
}  // namespace ficus

#endif /* ASYNC_COROUTINE_COROUTINE_TASK_QUEUE_ */
