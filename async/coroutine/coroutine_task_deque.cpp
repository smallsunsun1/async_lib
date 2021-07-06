#include "coroutine_task_deque.h"

#include "coroutine_thread_pool.h"

namespace sss {
namespace async {
namespace internal {

std::optional<ScheduleOperation *> CoroutineTaskDeque::PushFront(
    ScheduleOperation *task) {
  unsigned front = mFront.load(std::memory_order_relaxed);
  Elem *e = &mArray[front & kMask];
  uint8_t s = e->state.load(std::memory_order_relaxed);
  if (s != kEmpty ||
      !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire)) {
    return std::optional<ScheduleOperation *>(task);
  }
  mFront.store(front + kIncrement, std::memory_order_relaxed);
  e->task = task;
  e->state.store(kReady, std::memory_order_release);
  return std::nullopt;
}

unsigned CoroutineTaskDeque::CalculateSize(unsigned front,
                                           unsigned back) const {
  int size = (front & kMask2) - (back & kMask2);
  // Fix overflow.
  if (size < 0) size += 2 * kCapacity;
  // Order of modification in push/pop is crafted to make the queue look
  // larger than it is during concurrent modifications. E.g. push can
  // increment size before the corresponding pop has decremented it.
  // So the computed size can be up to kCapacity + 1, fix it.
  assert(size <= kCapacity + 1);
  if (size > static_cast<int>(kCapacity)) size = kCapacity;
  return static_cast<unsigned>(size);
}

std::optional<ScheduleOperation *> CoroutineTaskDeque::PopFront() {
  unsigned front = mFront.load(std::memory_order_relaxed);
  Elem *e = &mArray[(front - 1) & kMask];
  uint8_t s = e->state.load(std::memory_order_relaxed);
  if (s != kReady ||
      !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire)) {
    return std::nullopt;
  }
  ScheduleOperation *task = e->task;
  e->state.store(kEmpty, std::memory_order_release);
  front = ((front - 1) & kMask2) | (front & ~kMask2);
  mFront.store(front, std::memory_order_relaxed);
  return std::optional<ScheduleOperation *>(task);
}

std::optional<ScheduleOperation *> CoroutineTaskDeque::PushBack(
    ScheduleOperation *task) {
  std::lock_guard<std::mutex> lock(mMutex);
  unsigned back = mBack.load(std::memory_order_relaxed);
  Elem *e = &mArray[(back - 1) & kMask];
  uint8_t s = e->state.load(std::memory_order_relaxed);
  if (s != kEmpty ||
      !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire)) {
    return std::optional<ScheduleOperation *>(task);
  }
  back = ((back - 1) & kMask2) | (back & ~kMask2);
  mBack.store(back, std::memory_order_relaxed);
  e->task = task;
  e->state.store(kReady, std::memory_order_release);
  return std::nullopt;
}

std::optional<ScheduleOperation *> CoroutineTaskDeque::PopBack() {
  if (Empty()) return std::nullopt;

  std::lock_guard<std::mutex> lock(mMutex);
  unsigned back = mBack.load(std::memory_order_relaxed);
  Elem *e = &mArray[back & kMask];
  uint8_t s = e->state.load(std::memory_order_relaxed);
  if (s != kReady ||
      !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire)) {
    return std::nullopt;
  }
  ScheduleOperation *task = std::move(e->task);
  e->state.store(kEmpty, std::memory_order_release);
  mBack.store(back + kIncrement, std::memory_order_relaxed);
  return std::optional<ScheduleOperation *>(std::move(task));
}

unsigned CoroutineTaskDeque::PopBackHalf(
    std::vector<ScheduleOperation *> *result) {
  if (Empty()) return 0;

  std::lock_guard<std::mutex> lock(mMutex);
  unsigned back = mBack.load(std::memory_order_relaxed);
  unsigned size = Size();
  unsigned mid = back;
  if (size > 1) mid = back + (size - 1) / 2;
  unsigned n = 0;
  unsigned start = 0;
  for (; static_cast<int>(mid - back) >= 0; mid--) {
    Elem *e = &mArray[mid & kMask];
    uint8_t s = e->state.load(std::memory_order_relaxed);
    if (n == 0) {
      if (s != kReady || !e->state.compare_exchange_strong(
                             s, kBusy, std::memory_order_acquire))
        continue;
      start = mid;
    } else {
      // Note: no need to store temporal kBusy, we exclusively own these
      // elements.
      assert(s == kReady);
    }
    result->push_back(std::move(e->task));
    e->state.store(kEmpty, std::memory_order_release);
    n++;
  }
  if (n != 0) mBack.store(start + kIncrement, std::memory_order_relaxed);
  return n;
}

}  // namespace internal
}  // namespace async
}  // namespace sss