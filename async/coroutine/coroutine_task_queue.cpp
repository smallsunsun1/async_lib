#include "coroutine_task_queue.h"

#include "coroutine_thread_pool.h"

const unsigned sss::async::internal::CoroutineTaskQueue::kCapacity;

namespace sss {
namespace async {
namespace internal {

std::optional<ScheduleOperation *> CoroutineTaskQueue::PushFront(
    ScheduleOperation *task) {
  unsigned front = mFront.load(std::memory_order_relaxed);
  Elem *e;
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
    if (diff < 0) return {task};

    // Another thread acquired element at `front` index.
    if (diff > 0) front = mFront.load(std::memory_order_relaxed);
  }
  e->task = task;
  e->state.store(front + 1, std::memory_order_release);
  return std::nullopt;
}
std::optional<ScheduleOperation *> CoroutineTaskQueue::PopBack() {
  unsigned back = mBack.load(std::memory_order_relaxed);
  Elem *e;

  for (;;) {
    e = &mArray[back & kMask];
    unsigned state = e->state.load(std::memory_order_acquire);
    int64_t diff = static_cast<int64_t>(state) - static_cast<int64_t>(back + 1);

    // Element at `back` is ready, try to acquire its ownership.
    if (diff == 0 && mBack.compare_exchange_strong(back, back + 1,
                                                   std::memory_order_relaxed)) {
      break;
    }

    // We've reached an empty element.
    if (diff < 0) return std::nullopt;

    // Another thread popped a task from element at 'back'.
    if (diff > 0) back = mBack.load(std::memory_order_relaxed);
  }

  ScheduleOperation *task = e->task;
  e->state.store(back + kCapacity, std::memory_order_release);
  return {task};
}

unsigned CoroutineTaskQueue::Size() const {
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

}  // namespace internal
}  // namespace async
}  // namespace sss