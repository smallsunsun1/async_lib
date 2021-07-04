#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_BLOCKING_WORK_QUEUE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_BLOCKING_WORK_QUEUE_

#include <limits>
#include <list>
#include <queue>
#include <ratio>

#include "async/concurrent/task_queue.h"
#include "async/concurrent/work_queue_base.h"
#include "async/context/task_function.h"

namespace sss {
namespace async {
namespace internal {

template <typename ThreadingEnvironment>
class BlockingWorkQueue;

template <typename ThreadingEnvironmentTy>
struct WorkQueueTraits<BlockingWorkQueue<ThreadingEnvironmentTy>> {
  using ThreadingEnvironment = ThreadingEnvironmentTy;
  using Thread = typename ThreadingEnvironment::Thread;
  using Queue = ::sss::async::internal::TaskQueue;
};

template <typename ThreadingEnvironment>
class BlockingWorkQueue : public WorkQueueBase<BlockingWorkQueue<ThreadingEnvironment>> {
  using Base = WorkQueueBase<BlockingWorkQueue<ThreadingEnvironment>>;

  using Queue = typename Base::Queue;
  using Thread = typename Base::Thread;
  using PerThread = typename Base::PerThread;
  using ThreadData = typename Base::ThreadData;

 public:
  explicit BlockingWorkQueue(QuiescingState* quiescingState, size_t numThreads, size_t maxNumDynamicThreads = std::numeric_limits<size_t>::max(),
                             std::chrono::nanoseconds idleWaitTime = std::chrono::seconds(1));
  ~BlockingWorkQueue() { Quiesce(); }

  // Enqueues `task` for execution by one of the statically allocated thread.
  // Return task wrapped in optional if all per-thread queues are full.
  std::optional<TaskFunction> EnqueueBlockingTask(TaskFunction task);

  // Runs `task` in one of the dynamically started threads. Returns task
  // wrapped in optional if can't assign it to a worker thread.
  std::optional<TaskFunction> RunBlockingTask(TaskFunction task);

  void Quiesce();

 private:
  static constexpr char const* kThreadNamePrefix = "async-blocking-queue";
  static constexpr char const* kDynamicThreadNamePrefix = "async-dynamic-queue";

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

  std::optional<TaskFunction> NextTask(Queue* queue);
  std::optional<TaskFunction> Steal(Queue* queue);
  bool Empty(Queue* queue);

  // If the blocking task does not allow queuing, it is executed in one of the
  // dynamically spawned threads. These threads have 1-to-1 task-to-thread
  // relationship, and can guarantee that tasks with inter-dependencies
  // will all make progress together.

  // Waits for the next available task. Returns empty optional if the task was
  // not found.
  std::optional<TaskFunction> WaitNextTask(std::unique_lock<std::mutex>* lock);

  // Maximum number of dynamically started threads.
  const uint64_t mMaxNumDynamicThreads;

  // For how long dynamically started thread waits for the next task before
  // stopping.
  const std::chrono::nanoseconds mIdleWaitTime;

  // All operations with dynamic threads are done holding this mutex.
  std::mutex mMutex;
  std::condition_variable mWakeDoWorkCV;
  std::condition_variable mThreadExitedCV;

  // Number of started dynamic threads.
  size_t mNumDynamicThreads = 0;

  // Number of dynamic threads waiting for the next task.
  size_t mNumIdleDynamicThreads = 0;

  // This queue is a temporary storage to transfer task ownership to one of the
  // idle threads. It does not keep more tasks than there are idle threads.
  std::queue<TaskFunction> mIdleTaskQueue;

  // Unique pointer owning a dynamic thread, and an active flag.
  using DynamicThread = std::pair<std::unique_ptr<Thread>, bool>;

  // Container for dynamically started threads. Some of the threads might be
  // already terminated. Terminated threads lazily removed from the
  // `mDynamicThreads` on each call to `RunBlockingTask`.
  std::list<DynamicThread> mDynamicThreads;

  // Idle threads must stop waiting for the next task in the `mIdleTaskQueue`.
  bool mStopWaiting = false;
};

template <typename ThreadingEnvironment>
BlockingWorkQueue<ThreadingEnvironment>::BlockingWorkQueue(QuiescingState* quiescingState, uint64_t numThreads, uint64_t maxNumDynamicThreads, std::chrono::nanoseconds idleWaitTime)
    : WorkQueueBase<BlockingWorkQueue>(quiescingState, kThreadNamePrefix, numThreads), mMaxNumDynamicThreads(maxNumDynamicThreads), mIdleWaitTime(idleWaitTime) {}

template <typename ThreadingEnvironment>
std::optional<TaskFunction> BlockingWorkQueue<ThreadingEnvironment>::EnqueueBlockingTask(TaskFunction task) {
  // In quiescing mode we count the number of pending tasks, and are allowed to
  // execute tasks in the caller thread.
  const bool isQuiescing = IsQuiescing();
  if (isQuiescing) task = WithPendingTaskCounter(std::move(task));

  // If the worker queue is full, we will return `task` to the caller.
  std::optional<TaskFunction> inlineTask = {std::move(task)};

  PerThread* pt = GetPerThread();
  if (pt->parent == this) {
    // Worker thread of this pool, push onto the thread's queue.
    Queue& q = mThreadData[pt->thread_id].queue;
    inlineTask = q.PushFront(std::move(*inlineTask));
  } else {
    // A random free-standing thread (or worker of another pool).
    unsigned r = pt->rng();
    unsigned victim = FastReduce(r, mNumThreads);
    unsigned inc = mCoprimes[FastReduce(r, mCoprimes.size())];

    for (unsigned i = 0; i < mNumThreads && inlineTask.has_value(); i++) {
      inlineTask = mThreadData[victim].queue.PushFront(std::move(*inlineTask));
      if ((victim += inc) >= mNumThreads) victim -= mNumThreads;
    }
  }

  // Failed to push task into one of the worker threads queues.
  if (inlineTask.has_value()) {
    // If we are in quiescing mode, we can always execute the submitted task in
    // the caller thread, because the system is anyway going to shutdown soon,
    // and even if we are running inside a non-blocking work queue, a single
    // potential context switch won't negatively impact system performance.
    if (isQuiescing) {
      (*inlineTask)();
      return std::nullopt;
    } else {
      return inlineTask;
    }
  }

  // Note: below we touch `*this` after making `task` available to worker
  // threads. Strictly speaking, this can lead to a racy-use-after-free.
  // Consider that Schedule is called from a thread that is neither main thread
  // nor a worker thread of this pool. Then, execution of `task` directly or
  // indirectly completes overall computations, which in turn leads to
  // destruction of this. We expect that such a scenario is prevented by the
  // program, that is, this is kept alive while any threads can potentially be
  // in Schedule.
  if (IsNotifyParkedThreadRequired()) mEventCount.Notify(false);

  return std::nullopt;
}

template <typename ThreadingEnvironment>
std::optional<TaskFunction> BlockingWorkQueue<ThreadingEnvironment>::RunBlockingTask(TaskFunction task) {
  std::unique_lock<std::mutex> lock(mMutex);

  // Attach a PendingTask counter only if we were able to submit the task
  // to one of the worker threads. It's unsafe to return the task with
  // a counter to the caller, because we don't know when/if it will be
  // destructed and the counter decremented.
  auto wrap = [&](TaskFunction task) -> TaskFunction { return IsQuiescing() ? WithPendingTaskCounter(std::move(task)) : std::move(task); };

  // There are idle threads. We enqueue the task to the queue and then notify
  // one of the idle threads.
  if (mIdleTaskQueue.size() < mNumIdleDynamicThreads) {
    mIdleTaskQueue.emplace(wrap(std::move(task)));
    mWakeDoWorkCV.notify_one();

    return std::nullopt;
  }

  // Cleanup dynamic threads that are already terminated.
  mDynamicThreads.remove_if([](DynamicThread& thread) -> bool { return thread.second == false; });

  // There are no idle threads and we are not at the thread limit. We
  // start a new thread to run the task.
  if (mNumDynamicThreads < mMaxNumDynamicThreads) {
    // Prepare an entry to hold a new dynamic thread.
    //
    // NOTE: We rely on std::list pointer stability for passing a reference to
    // the container element to the `do_work` lambda.
    mDynamicThreads.emplace_back();
    DynamicThread& dynamicThread = mDynamicThreads.back();

    auto do_work = [this, &dynamicThread, task = wrap(std::move(task))]() mutable {
      task();
      // Reset executed task to call destructor without holding the lock,
      // because it might be expensive. Also we want to call it before
      // notifying quiescing thread, because destructor potentially could
      // drop the last references on captured async values.
      task.reset();

      std::unique_lock<std::mutex> lock(mMutex);

      // Try to get the next task. If one is found, run it. If there is no
      // task to execute, GetNextTask will return None that converts to
      // false.
      while (std::optional<TaskFunction> task = WaitNextTask(&lock)) {
        mMutex.unlock();
        // Do not hold the lock while executing and destructing the task.
        (*task)();
        task.reset();
        mMutex.lock();
      }

      // No more work to do or shutdown occurred. Exit the thread.
      dynamicThread.second = false;
      --mNumDynamicThreads;
      if (mStopWaiting) mThreadExitedCV.notify_one();
    };

    // Start a new dynamic thread.
    dynamicThread.second = true;  // is active
    dynamicThread.first = ThreadingEnvironment::StartThread(std::move(do_work));
    ++mNumDynamicThreads;

    return std::nullopt;
  }

  // There are no idle threads and we are at the thread limit. Return task
  // to the caller.
  return {std::move(task)};
}

template <typename ThreadingEnvironment>
std::optional<TaskFunction> BlockingWorkQueue<ThreadingEnvironment>::WaitNextTask(std::unique_lock<std::mutex>* lock) {
  ++mNumIdleDynamicThreads;

  const auto timeout = std::chrono::system_clock::now() + mIdleWaitTime;
  mWakeDoWorkCV.wait_until(*lock, timeout, [this]() { return !mIdleTaskQueue.empty() || mStopWaiting; });
  --mNumIdleDynamicThreads;

  // Found something in the queue. Return the task.
  if (!mIdleTaskQueue.empty()) {
    TaskFunction task = std::move(mIdleTaskQueue.front());
    mIdleTaskQueue.pop();
    return {std::move(task)};
  }

  // Shutdown occurred. Return empty optional.
  return std::nullopt;
}

template <typename ThreadingEnvironment>
void BlockingWorkQueue<ThreadingEnvironment>::Quiesce() {
  Base::Quiesce();

  // WARN: This function provides only best-effort work queue emptyness
  // guarantees. Tasks running inside a dynamically allocated threads
  // potentially could submit new tasks to statically allocated threads, and
  // current implementaton will miss them. Clients must rely on
  // MultiThreadedWorkQueue::Quiesce() for strong emptyness guarantees.

  // Wait for the completion of all tasks in the dynamicly part of a queue.
  std::unique_lock<std::mutex> lock(mMutex);

  // Wake up all idle threads.
  mStopWaiting = true;
  mWakeDoWorkCV.notify_all();

  // Wait until all dynamicaly started threads stopped.
  mThreadExitedCV.wait(lock, [this]() { return mNumDynamicThreads == 0; });
  assert(mIdleTaskQueue.empty());

  // Prepare for the next call to Quiesce.
  mStopWaiting = false;
}

template <typename ThreadingEnvironment>
std::optional<TaskFunction> BlockingWorkQueue<ThreadingEnvironment>::NextTask(Queue* queue) {
  return queue->PopBack();
}

template <typename ThreadingEnvironment>
std::optional<TaskFunction> BlockingWorkQueue<ThreadingEnvironment>::Steal(Queue* queue) {
  return queue->PopBack();
}

template <typename ThreadingEnvironment>
bool BlockingWorkQueue<ThreadingEnvironment>::Empty(Queue* queue) {
  return queue->Empty();
}

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_BLOCKING_WORK_QUEUE_ \
        */
