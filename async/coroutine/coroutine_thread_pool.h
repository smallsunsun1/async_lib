#ifndef ASYNC_COROUTINE_COROUTINE_THREAD_POOL_
#define ASYNC_COROUTINE_COROUTINE_THREAD_POOL_

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <mutex>

#include "async/concurrent/blocking_work_queue.h"
#include "async/concurrent/concurrent_work_queue.h"
#include "async/concurrent/environment.h"
#include "async/concurrent/non_blocking_work_queue.h"
#include "async/support/string_util.h"

namespace ficus {
namespace async {

class CoroutineThreadPool;

class ScheduleOperation {
 public:
  ScheduleOperation(CoroutineThreadPool* pool) : mThreadPool(pool) {}
  bool await_ready() noexcept { return false; }
  bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
  void await_resume() noexcept {}

 private:
  friend class CoroutineThreadPool;
  CoroutineThreadPool* mThreadPool;
  std::coroutine_handle<> mAwaitingCoroutine;
  ScheduleOperation* mNext;
};

class CoroutineThreadPool : public ConcurrentWorkQueue {
  using ThreadingEnvironment = internal::StdThreadingEnvironment;

 public:
  ~CoroutineThreadPool() override;
  explicit CoroutineThreadPool(int numThreads, int maxBlockingWorkQueueThread);
  std::string name() const override { return StrCat("CoroutineThreadPool threads:  (", mNumThreads, " threads)"); }
  int GetParallelismLevel() const final { return mNumThreads; }
  ScheduleOperation Schedule() noexcept { return ScheduleOperation{this}; }

 private:
  void ScheduleImpl(ScheduleOperation* operation) noexcept;
  const uint32_t mNumThreads;
  std::unique_ptr<internal::QuiescingState> mQuiescingState;
  internal::NonBlockingWorkQueue<ThreadingEnvironment> mNonBlockingWorkQueue;
  internal::BlockingWorkQueue<ThreadingEnvironment> mBlockingWorkQueue;
};

}  // namespace async
}  // namespace ficus

#endif /* ASYNC_COROUTINE_COROUTINE_THREAD_POOL_ */
