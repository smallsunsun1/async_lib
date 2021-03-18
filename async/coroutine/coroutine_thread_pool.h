#ifndef ASYNC_COROUTINE_COROUTINE_THREAD_POOL_
#define ASYNC_COROUTINE_COROUTINE_THREAD_POOL_

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <mutex>

#include "async/concurrent/blocking_work_queue.h"
#include "async/concurrent/concurrent_work_queue.h"
#include "async/concurrent/environment.h"
#include "async/concurrent/work_queue_base.h"
#include "async/coroutine/coroutine_task_deque.h"
#include "async/coroutine/coroutine_task_queue.h"
#include "async/support/string_util.h"

namespace sss {
namespace async {

class CoroutineThreadPool;

class ScheduleOperation {
 public:
  ScheduleOperation(CoroutineThreadPool* pool) : mThreadPool(pool) {}
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
  void await_resume() noexcept {}

 private:
  friend class CoroutineThreadPool;
  CoroutineThreadPool* mThreadPool;
  std::coroutine_handle<> mAwaitingCoroutine;
  ScheduleOperation* mNext;
};

class CoroutineThreadPool {
  using ThreadingEnvironment = internal::StdThreadingEnvironment;
  using Queue = internal::CoroutineTaskDeque;
  using Thread = internal::StdThreadingEnvironment::Thread;

 public:
  ~CoroutineThreadPool() {
    mCancelled.store(true, std::memory_order_relaxed);
    for (auto& thread : mThreadData) {
      thread.thread->join();
    }
  }
  explicit CoroutineThreadPool(int numThreads) : mNumThreads(numThreads), mCancelled(false), mCoprimes(internal::ComputeCoprimes(numThreads)), mThreadData(numThreads) {
    assert(numThreads >= 1 && "thread number must be larger than 1");
    for (int i = 0; i < numThreads; ++i) {
      mThreadData[i].thread = ThreadingEnvironment::StartThread([this, i]() { WorkerLoop(i); });
    }
  }
  void WorkerLoop(int threadId);
  std::string name() const { return StrCat("CoroutineThreadPool threads:  (", mNumThreads, " threads)"); }
  int GetParallelismLevel() const { return mNumThreads; }
  ScheduleOperation Schedule() noexcept { return ScheduleOperation{this}; }
  void AddTask(ScheduleOperation* task);
  absl::optional<ScheduleOperation*> NextTask(Queue* queue);
  absl::optional<ScheduleOperation*> Steal(Queue* queue);
  absl::optional<ScheduleOperation*> Steal();

 private:
  friend class ScheduleOperation;
  CoroutineThreadPool(const CoroutineThreadPool&) = delete;
  CoroutineThreadPool& operator=(const CoroutineThreadPool&) = delete;
  void ScheduleImpl(ScheduleOperation* operation) noexcept;
  const uint32_t mNumThreads;
  struct PerThread {
    constexpr PerThread() : parent(nullptr), rng(0), thread_id(-1) {}
    CoroutineThreadPool* parent;
    internal::FastRng rng;  // Random number generator
    int thread_id;          // Worker thread index in the workers queue
  };
  struct ThreadData {
    ThreadData() : thread(), queue() {}
    std::unique_ptr<Thread> thread;
    Queue queue;
  };
  std::vector<ThreadData> mThreadData;
  std::vector<unsigned> mCoprimes;
  std::atomic<bool> mCancelled;
  static PerThread* GetPerThread() {
    static thread_local PerThread perThread;
    PerThread* pt = &perThread;
    return pt;
  }
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_COROUTINE_THREAD_POOL_ */
