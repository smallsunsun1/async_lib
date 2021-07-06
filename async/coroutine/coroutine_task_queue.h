#ifndef ASYNC_COROUTINE_COROUTINE_TASK_QUEUE_
#define ASYNC_COROUTINE_COROUTINE_TASK_QUEUE_

#include <array>
#include <atomic>
#include <cassert>
#include <limits>
#include <mutex>
#include <optional>

namespace sss {
namespace async {

class ScheduleOperation;

namespace internal {

class CoroutineTaskQueue {
 public:
  static const unsigned kCapacity = 1024;
  static_assert((kCapacity > 2) && (kCapacity <= (64u << 10u)),
                "CoroutineTaskQueue capacity must be in [4, 65536] range");
  static_assert(
      (kCapacity & (kCapacity - 1)) == 0,
      "CoroutineTaskQueue capacity must be a power of two for fast masking");
  CoroutineTaskQueue() : mFront(0), mBack(0) {
    for (unsigned i = 0; i < kCapacity; ++i) mArray[i].state.store(i);
  }
  CoroutineTaskQueue(const CoroutineTaskQueue &) = delete;
  CoroutineTaskQueue &operator=(const CoroutineTaskQueue &) = delete;
  std::optional<ScheduleOperation *> PushFront(ScheduleOperation *task);
  std::optional<ScheduleOperation *> PopBack();
  unsigned Size() const;
  bool Empty() const { return Size() == 0; }
  void Flush() {
    while (!Empty()) {
      std::optional<ScheduleOperation *> task = PopBack();
      assert(task.has_value());
    }
  }

 private:
  static constexpr unsigned kMask = kCapacity - 1;
  struct Elem {
    std::atomic<unsigned> state;
    ScheduleOperation *task;
  };
  // Align to 128 byte boundary to prevent false sharing between front and back
  // pointers, they are accessed from different threads.
  alignas(128) std::atomic<unsigned> mFront;
  alignas(128) std::atomic<unsigned> mBack;

  std::array<Elem, kCapacity> mArray;
};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_COROUTINE_TASK_QUEUE_ */
