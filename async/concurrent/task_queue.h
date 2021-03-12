#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_TASK_QUEUE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_TASK_QUEUE_

#include <array>
#include <atomic>
#include <cassert>
#include <limits>
#include <mutex>

#include "../context/task_function.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ficus {
namespace async {
namespace internal {

class TaskQueue {
 public:
  static const unsigned kCapacity = 1024;

  static_assert((kCapacity > 2) && (kCapacity <= (64u << 10u)),
                "TaskQueue capacity must be in [4, 65536] range");
  static_assert((kCapacity & (kCapacity - 1)) == 0,
                "TaskQueue capacity must be a power of two for fast masking");

  TaskQueue() : mFront(0), mBack(0) {
    for (unsigned i = 0; i < kCapacity; ++i) mArray[i].state.store(i);
  }

  TaskQueue(const TaskQueue&) = delete;
  void operator=(const TaskQueue&) = delete;

  ~TaskQueue() { assert(Empty()); }

  // PushFront() inserts task at the beginning of the queue.
  //
  // If the queue is full, returns passed in task wrapped in optional, otherwise
  // returns empty optional.
  absl::optional<TaskFunction> PushFront(TaskFunction task) {
    unsigned front = mFront.load(std::memory_order_relaxed);
    Elem* e;

    for (;;) {
      e = &mArray[front & kMask];
      unsigned state = e->state.load(std::memory_order_acquire);
      int64_t diff = static_cast<int64_t>(state) - static_cast<int64_t>(front);

      // Try to acquire an ownership of element at `front`.
      if (diff == 0 && mFront.compare_exchange_strong(
                           front, front + 1, std::memory_order_relaxed)) {
        break;
      }

      // We wrapped around the queue, and have no space in the queue.
      if (diff < 0) return {std::move(task)};

      // Another thread acquired element at `front` index.
      if (diff > 0) front = mFront.load(std::memory_order_relaxed);
    }

    // Move the task to the acquired element.
    e->task = std::move(task);
    e->state.store(front + 1, std::memory_order_release);

    return absl::nullopt;
  }

  // PopBack() removes and returns the last elements in the queue.
  //
  // If the queue is empty returns empty optional.
  absl::optional<TaskFunction> PopBack() {
    unsigned back = mBack.load(std::memory_order_relaxed);
    Elem* e;

    for (;;) {
      e = &mArray[back & kMask];
      unsigned state = e->state.load(std::memory_order_acquire);
      int64_t diff =
          static_cast<int64_t>(state) - static_cast<int64_t>(back + 1);

      // Element at `back` is ready, try to acquire its ownership.
      if (diff == 0 && mBack.compare_exchange_strong(
                           back, back + 1, std::memory_order_relaxed)) {
        break;
      }

      // We've reached an empty element.
      if (diff < 0) return absl::nullopt;

      // Another thread popped a task from element at 'back'.
      if (diff > 0) back = mBack.load(std::memory_order_relaxed);
    }

    TaskFunction task = std::move(e->task);
    e->state.store(back + kCapacity, std::memory_order_release);

    return {std::move(task)};
  }

  bool Empty() const { return Size() == 0; }

  // Size() returns a queue size estimate that potentially could be larger
  // than the real number of tasks in the queue. It never can be smaller.
  unsigned Size() const {
    // Emptiness plays critical role in thread pool blocking. So we go to great
    // effort to not produce false positives (claim non-empty queue as empty).
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

  // Delete all the elements from the queue.
  void Flush() {
    while (!Empty()) {
      absl::optional<TaskFunction> task = PopBack();
      assert(task.has_value());
    }
  }

 private:
  // Mask for extracting front and back pointer positions.
  static constexpr unsigned kMask = kCapacity - 1;

  struct Elem {
    std::atomic<unsigned> state;
    TaskFunction task;
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

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_TASK_QUEUE_ */
