#ifndef ASYNC_COROUTINE_NON_BLOCKING_COROUTINE_QUEUE_
#define ASYNC_COROUTINE_NON_BLOCKING_COROUTINE_QUEUE_

#include "async/concurrent/work_queue_base.h"
#include "coroutine_task_deque.h"

namespace ficus {
namespace async {
namespace internal {

template <typename ThreadingEnvironment>
class NonBlockingCoroutineQueue;

template <typename ThreadingEnvironmentTy>
struct WorkQueueTraits<NonBlockingCoroutineQueue<ThreadingEnvironmentTy>> {
  using ThreadingEnvironment = ThreadingEnvironmentTy;
  using Thread = typename ThreadingEnvironment::Thread;
  using Queue = typename ::ficus::async::internal::CoroutineTaskDeque;
};

template <typename ThreadingEnvironment>
class NonBlockingCoroutineQueue : public WorkQueueBase<NonBlockingCoroutineQueue<ThreadingEnvironment>> {
  using Base = WorkQueueBase<NonBlockingCoroutineQueue<ThreadingEnvironment>>;
  using Queue = typename Base::Queue;
  using Thread = typename Base::Thread;
  using PerThread = typename Base::PerThread;
  using ThreadData = typename Base::ThreadData;

 public:
  explicit NonBlockingCoroutineQueue(QuiescingState* quiescingState, int numThreads);
  ~NonBlockingCoroutineQueue() = default;
  void AddTask(ScheduleOperation* task);
  using Base::Steal;

 private:
  static constexpr char const* kThreadNamePrefix = "async-non-blocking-coroutine-queue";
  template <typename WorkQueue>
  friend class WorkQueueBase;
  using Base::GetPerThread;
  using Base::IsNotifyParkedThreadRequired;
  using Base::IsQuiescing;
  using Base::WithPendingTaskCounter;

  using Base::mCoprimes;
  using Base::mEventCount;
  using Base::mNumThreads;
  using Base::mThreadData;
  absl::optional<ScheduleOperation*> NextTask(Queue* queue);
  absl::optional<ScheduleOperation*> Steal(Queue* queue);
  bool Empty(Queue* queue);
};

template <typename ThreadingEnvironment>
NonBlockingCoroutineQueue<ThreadingEnvironment>::NonBlockingCoroutineQueue(QuiescingState* quiescingState, int numThreads)
    : WorkQueueBase<NonBlockingCoroutineQueue>(quiescingState, kThreadNamePrefix, numThreads) {}
template <typename ThreadingEnvironment>
void NonBlockingCoroutineQueue<ThreadingEnvironment>::AddTask(ScheduleOperation* task) {
  if (IsQuiescing()) task = WithPendingTaskCounter(std::move(task));
}

}  // namespace internal
}  // namespace async
}  // namespace ficus

#endif /* ASYNC_COROUTINE_NON_BLOCKING_COROUTINE_QUEUE_ */
