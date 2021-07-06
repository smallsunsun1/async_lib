#include "coroutine_thread_pool.h"

#include "absl/synchronization/notification.h"

namespace sss {
namespace async {

void ScheduleOperation::await_suspend(
    std::coroutine_handle<> awaitingCoroutine) noexcept {
  mAwaitingCoroutine = awaitingCoroutine;
  mThreadPool->ScheduleImpl(this);
}

void CoroutineThreadPool::WorkerLoop(int threadId) {
  PerThread *pt = GetPerThread();
  pt->parent = this;
  pt->rng = internal::FastRng(ThreadingEnvironment::ThisThreadIdHash());
  pt->thread_id = threadId;
  Queue *q = &(mThreadData[threadId].queue);
  while (!mCancelled) {
    std::optional<ScheduleOperation *> t = NextTask(q);
    if (!t.has_value()) {
      t = Steal();
    }
    if (t.has_value()) {
      (*t)->mAwaitingCoroutine.resume();
    }
  }
}

void CoroutineThreadPool::ScheduleImpl(ScheduleOperation *operation) noexcept {
  AddTask(operation);
}

std::optional<ScheduleOperation *> CoroutineThreadPool::NextTask(Queue *queue) {
  return queue->PopFront();
}

std::optional<ScheduleOperation *> CoroutineThreadPool::Steal(Queue *queue) {
  return queue->PopBack();
}

std::optional<ScheduleOperation *> CoroutineThreadPool::Steal() {
  PerThread *pt = GetPerThread();
  unsigned r = pt->rng();
  unsigned victim = internal::FastReduce(r, mNumThreads);
  unsigned inc = mCoprimes[internal::FastReduce(r, mCoprimes.size())];
  for (unsigned i = 0; i < mNumThreads; i++) {
    std::optional<ScheduleOperation *> t = Steal(&(mThreadData[victim].queue));
    if (t.has_value()) return t;

    victim += inc;
    if (victim >= mNumThreads) {
      victim -= mNumThreads;
    }
  }
  return std::nullopt;
}

void CoroutineThreadPool::AddTask(ScheduleOperation *task) {
  PerThread *pt = GetPerThread();
  std::optional<ScheduleOperation *> inlineTask;
  if (pt->parent == this) {
    // 属于当前线程池的worker
    Queue &q = mThreadData[pt->thread_id].queue;
    inlineTask = q.PushFront(std::move(task));
  } else {
    unsigned rnd = internal::FastReduce(pt->rng(), mNumThreads);
    Queue &q = mThreadData[rnd].queue;
    inlineTask = q.PushBack(std::move(task));
  }
  if (inlineTask.has_value()) {
    (*inlineTask)->mAwaitingCoroutine.resume();
  }
}

}  // namespace async
}  // namespace sss
