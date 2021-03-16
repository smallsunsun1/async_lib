#include "coroutine_thread_pool.h"
#include "absl/synchronization/notification.h"

void ScheduleOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    mAwaitingCoroutine = awaitingCoroutine;
    mThreadPool->ScheduleImpl(this);
}

void CoroutineThreadPool::WorkerLoop(int threadId) {
  PerThread* pt = GetPerThread();
  pt->parent = &mDerived;
  pt->rng = FastRng(ThreadingEnvironment::ThisThreadIdHash());
  pt->thread_id = thread_id;
  Queue* q = &(mThreadData[thread_id].queue);
  while (!mCancelled) {
    absl::optional<ScheduleOperation*> t = NextTask(q);
    if (!t.has_value()) {
        t = Steal();
    }
    if (t.has_value()) {
        (*t)->mAwaitingCoroutine.resume();
    }
  }
}

void CoroutineThreadPool::ScheduleImpl(ScheduleOperation* operation) noexcept {
    AddTask(operation);
}

absl::optional<ScheduleOperation*> CoroutineThreadPool::NextTask(Queue* queue) {
    return queue->PopFront();
}

absl::optional<ScheduleOperation*> CoroutineThreadPool::Steal(Queue* queue) {
    return queue->PopBack();
}

absl::optional<ScheduleOperation*> CoroutineThreadPool::Steal() {
  PerThread* pt = GetPerThread();
  unsigned r = pt->rng();
  unsigned victim = internal::FastReduce(r, mNumThreads);
  unsigned inc = mCoprimes[internal::FastReduce(r, mCoprimes.size())];
  for (unsigned i = 0; i < mNumThreads; i++) {
    absl::optional<ScheduleOperation*> t = Steal(&(mThreadData[victim].queue));
    if (t.has_value()) return t;

    victim += inc;
    if (victim >= mNumThreads) {
      victim -= mNumThreads;
    }
  }
  return absl::nullopt;
}

void CoroutineThreadPool::AddTask(ScheduleOperation* task) {
    PerThread* pt = GetPerThread();
    absl::optional<ScheduleOperation*> inlineTask;
    if (pt->parent == this) {
        // 属于当前线程池的worker
        Queue& q = mThreadData[pt->thread_id].queue;
        skipNotify = q.Empty();
        inlineTask = q.PushFront(std::move(task));
    } else {
        unsigned rnd = internal::FastReduce(pt->rng(), mNumThreads);
        Queue& q = mThreadData[rnd].queue;
        inlineTask = q.PushBack(std::move(task));
    }
    if (inlineTask.has_value()) {
        (*inlineTask)->mAwaitingCoroutine.resume();
    }
}

