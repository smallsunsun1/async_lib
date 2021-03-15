#include "async/context/async_value.h"
#include "async/support/latch.h"
#include "async/support/ref_count.h"
#include "async/support/string_util.h"
#include "blocking_work_queue.h"
#include "concurrent_work_queue.h"
#include "environment.h"
#include "non_blocking_work_queue.h"

namespace ficus {
namespace async {

class MultiThreadedWorkQueue : public ConcurrentWorkQueue {
  using ThreadingEnvironment = internal::StdThreadingEnvironment;

 public:
  MultiThreadedWorkQueue(int numThreads, int maxBlockingWorkQueueThread);
  ~MultiThreadedWorkQueue() override;

  std::string name() const override { return StrCat("Multi-threaded C++ work queue (", mNumThreads, " threads)"); }

  int GetParallelismLevel() const final { return mNumThreads; }

  void AddTask(TaskFunction task) final;
  absl::optional<TaskFunction> AddBlockingTask(TaskFunction task, bool allow_queuing) final;
  void Quiesce() final;
  void Await(absl::Span<const RCReference<AsyncValue>> values) final;
  bool IsInWorkerThread() const final;

 private:
  const int mNumThreads;
  std::unique_ptr<internal::QuiescingState> mQuiescingState;
  internal::NonBlockingWorkQueue<ThreadingEnvironment> mNonBlockingWorkQueue;
  internal::BlockingWorkQueue<ThreadingEnvironment> mBlockingWorkQueue;
};

MultiThreadedWorkQueue::MultiThreadedWorkQueue(int numThreads, int maxBlockingWorkQueueThread)
    : mNumThreads(numThreads),
      mQuiescingState(std::make_unique<internal::QuiescingState>()),
      mNonBlockingWorkQueue(mQuiescingState.get(), numThreads),
      mBlockingWorkQueue(mQuiescingState.get(), maxBlockingWorkQueueThread) {}
MultiThreadedWorkQueue::~MultiThreadedWorkQueue() {
  // Pending tasks in the underlying queues might submit new tasks to each other
  // during destruction.
  Quiesce();
}

void MultiThreadedWorkQueue::AddTask(TaskFunction task) { mNonBlockingWorkQueue.AddTask(std::move(task)); }

absl::optional<TaskFunction> MultiThreadedWorkQueue::AddBlockingTask(TaskFunction task, bool allow_queuing) {
  if (allow_queuing) {
    return mBlockingWorkQueue.EnqueueBlockingTask(std::move(task));
  } else {
    return mBlockingWorkQueue.RunBlockingTask(std::move(task));
  }
}

void MultiThreadedWorkQueue::Quiesce() {
  // Turn on pending tasks counter inside both work queues.
  auto quiescing = internal::Quiescing::Start(mQuiescingState.get());

  // We call NonBlockingWorkQueue::Quiesce() first because we prefer to keep
  // caller thread busy with compute intensive tasks.
  mNonBlockingWorkQueue.Quiesce();

  // Wait for completion of all blocking tasks.
  mBlockingWorkQueue.Quiesce();

  // At this point we might still have tasks in the work queues, but because we
  // enabled quiescing mode earlier, we can rely on empty check as a loop
  // condition.
  while (quiescing.HasPendingTasks()) {
    mNonBlockingWorkQueue.Quiesce();
    mBlockingWorkQueue.Quiesce();
  }
}

void MultiThreadedWorkQueue::Await(absl::Span<const RCReference<AsyncValue>> values) {
  // We might block on a latch waiting for the completion of all tasks, and
  // this is not allowed to do inside non blocking work queue.
  mNonBlockingWorkQueue.CheckCallerThread("MultiThreadedWorkQueue::Await");

  // We are done when valuesRemaining drops to zero.
  latch valuesRemaining(values.size());

  // As each value becomes available, we decrement the count.
  for (auto& value : values) {
    value->AndThen([&valuesRemaining]() { valuesRemaining.count_down(); });
  }

  // Wait until all values are resolved.
  valuesRemaining.wait();
}

bool MultiThreadedWorkQueue::IsInWorkerThread() const { return mNonBlockingWorkQueue.IsInWorkerThread(); }

std::unique_ptr<ConcurrentWorkQueue> CreateMultiThreadedWorkQueue(int numThreads, int numBlockingThreads) {
  assert(numThreads > 0 && numBlockingThreads > 0);
  return std::make_unique<MultiThreadedWorkQueue>(numThreads, numBlockingThreads);
}

}  // namespace async
}  // namespace ficus
