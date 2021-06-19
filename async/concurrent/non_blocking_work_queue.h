#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_NON_BLOCKING_WORK_QUEUE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_NON_BLOCKING_WORK_QUEUE_

#include "async/concurrent/task_deque.h"
#include "async/concurrent/work_queue_base.h"
#include "async/context/task_function.h"

namespace sss {
namespace async {
namespace internal {

template <typename ThreadingEnvironment>
class NonBlockingWorkQueue;

template <typename ThreadingEnvironmentTy>
struct WorkQueueTraits<NonBlockingWorkQueue<ThreadingEnvironmentTy>> {
  using ThreadingEnvironment = ThreadingEnvironmentTy;
  using Thread = typename ThreadingEnvironment::Thread;
  using Queue = typename ::sss::async::internal::TaskDeque;
};

template <typename ThreadingEnvironment>
class NonBlockingWorkQueue : public WorkQueueBase<NonBlockingWorkQueue<ThreadingEnvironment>> {
  using Base = WorkQueueBase<NonBlockingWorkQueue<ThreadingEnvironment>>;

  using Queue = typename Base::Queue;
  using Thread = typename Base::Thread;
  using PerThread = typename Base::PerThread;
  using ThreadData = typename Base::ThreadData;

 public:
  explicit NonBlockingWorkQueue(QuiescingState* quiescingState, int numThreads);
  ~NonBlockingWorkQueue() = default;

  void AddTask(TaskFunction task);

  using Base::Steal;

 private:
  static constexpr char const* kThreadNamePrefix = "async-non-blocking-queue";

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

  absl::optional<TaskFunction> NextTask(Queue* queue);
  absl::optional<TaskFunction> Steal(Queue* queue);
  bool Empty(Queue* queue);
};

template <typename ThreadingEnvironment>
NonBlockingWorkQueue<ThreadingEnvironment>::NonBlockingWorkQueue(QuiescingState* quiescingState, int numThreads) : WorkQueueBase<NonBlockingWorkQueue>(quiescingState, kThreadNamePrefix, numThreads) {}

template <typename ThreadingEnvironment>
void NonBlockingWorkQueue<ThreadingEnvironment>::AddTask(TaskFunction task) {
  // Keep track of the number of pending tasks.
  if (IsQuiescing()) task = WithPendingTaskCounter(std::move(task));

  // If the worker queue is full, we will execute `task` in the current thread.
  absl::optional<TaskFunction> inlineTask;

  // If a caller thread is managed by `this` we push the new task into the front
  // of thread own queue (LIFO execution order). PushFront is completely lock
  // free (PushBack requires a mutex lock), and improves data locality (in
  // practice tasks submitted together share some data).
  //
  // If a caller is a free-standing thread (or worker of another pool), we push
  // the new task into a random queue (FIFO execution order). Tasks still could
  // be executed in LIFO order, if they would be stolen by other workers.

  // We opportunistically skip notifying parked thread if we push a task to the
  // front of an empty queue. It is very common pattern when a task running in a
  // worker thread pushes continuation to the work queue, and finished its
  // execution shortly after (but we do not know this for sure at this point).
  // However if the task continues its execution for a long time after
  // submitting a continuation, this might lead to sub-optimal parallelization.
  bool skipNotify = false;

  PerThread* pt = GetPerThread();
  if (pt->parent == this) {
    // Worker thread of this pool, push onto the thread's queue.
    Queue& q = mThreadData[pt->thread_id].queue;
    skipNotify = q.Empty();
    inlineTask = q.PushFront(std::move(task));
  } else {
    // A free-standing thread (or worker of another pool).
    unsigned rnd = FastReduce(pt->rng(), mNumThreads);
    Queue& q = mThreadData[rnd].queue;
    inlineTask = q.PushBack(std::move(task));
  }
  // Note: below we touch `*this` after making `task` available to worker
  // threads. Strictly speaking, this can lead to a racy-use-after-free.
  // Consider that Schedule is called from a thread that is neither main thread
  // nor a worker thread of this pool. Then, execution of `task` directly or
  // indirectly completes overall computations, which in turn leads to
  // destruction of this. We expect that such a scenario is prevented by the
  // program, that is, this is kept alive while any threads can potentially be
  // in Schedule.
  if (!inlineTask.has_value()) {
    if (!skipNotify && IsNotifyParkedThreadRequired()) mEventCount.Notify(/*notify_all=*/false);
  } else {
    (*inlineTask)();  // Push failed, execute directly.
  }
}

template <typename ThreadingEnvironment>
absl::optional<TaskFunction> NonBlockingWorkQueue<ThreadingEnvironment>::NextTask(Queue* queue) {
  return queue->PopFront();
}

template <typename ThreadingEnvironment>
absl::optional<TaskFunction> NonBlockingWorkQueue<ThreadingEnvironment>::Steal(Queue* queue) {
  return queue->PopBack();
}

template <typename ThreadingEnvironment>
bool NonBlockingWorkQueue<ThreadingEnvironment>::Empty(Queue* queue) {
  return queue->Empty();
}

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_NON_BLOCKING_WORK_QUEUE_ \
        */
