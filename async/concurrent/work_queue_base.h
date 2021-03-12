#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_WORK_QUEUE_BASE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_WORK_QUEUE_BASE_

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "../context/task_function.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "event_count.h"

namespace ficus {
namespace async {
namespace internal {
struct FastRng {
  constexpr explicit FastRng(uint64_t state) : state(state) {}
  unsigned operator()() {
    uint64_t current = state;
    // Update the internal state
    state = current * 6364136223846793005ULL + 0xda3e39cb94b95bdbULL;
    // Generate the random output (using the PCG-XSH-RS scheme)
    return static_cast<unsigned>((current ^ (current >> 22u)) >>
                                 (22 + (current >> 61u)));
  }
  uint64_t state;
};
inline uint32_t FastReduce(uint32_t x, uint32_t size) {
  return (static_cast<uint64_t>(x) * static_cast<uint64_t>(size)) >> 32u;
}
template <typename Derived>
struct WorkQueueTraits;
struct QuiescingState {
  std::atomic<int64_t> mNumQuiescing;
  std::atomic<int64_t> mNumPendingTasks;
};
class Quiescing {
 public:
  static Quiescing Start(QuiescingState* state) { return Quiescing(state); }
  explicit Quiescing(QuiescingState* state) : mState(state) {
    assert(state != nullptr);
    mState->mNumQuiescing.fetch_add(1, std::memory_order_relaxed);
  }
  ~Quiescing() {
    if (mState == nullptr) return;  // in moved-out state
    mState->mNumQuiescing.fetch_sub(1, std::memory_order_relaxed);
  }

  Quiescing(Quiescing&& other) : mState(other.mState) {
    other.mState = nullptr;
  }

  Quiescing& operator=(Quiescing&& other) {
    mState = other.mState;
    other.mState = nullptr;
    return *this;
  }

  Quiescing(const Quiescing&) = delete;
  Quiescing& operator=(const Quiescing&) = delete;

  // HasPendingTasks() returns true if some of the tasks added to the owning
  // queue after `*this` was created are not completed.
  bool HasPendingTasks() const {
    return mState->mNumPendingTasks.load(std::memory_order_relaxed) != 0;
  }

 private:
  QuiescingState* mState;
};
class PendingTask {
 public:
 public:
  explicit PendingTask(QuiescingState* state) : mState(state) {
    assert(state != nullptr);
    mState->mNumPendingTasks.fetch_add(1, std::memory_order_relaxed);
  }

  ~PendingTask() {
    if (mState == nullptr) return;  // in moved-out state
    mState->mNumPendingTasks.fetch_sub(1, std::memory_order_relaxed);
  }

  PendingTask(PendingTask&& other) : mState(other.mState) {
    other.mState = nullptr;
  }

  PendingTask& operator=(PendingTask&& other) {
    mState = other.mState;
    other.mState = nullptr;
    return *this;
  }

  PendingTask(const PendingTask&) = delete;
  PendingTask& operator=(const PendingTask&) = delete;

 private:
  QuiescingState* mState;
};

template <typename Derived>
class WorkQueueBase {
  using Traits = WorkQueueTraits<Derived>;
  using Queue = typename Traits::Queue;
  using Thread = typename Traits::Thread;
  using ThreadingEnvironment = typename Traits::ThreadingEnvironment;

 public:
  bool IsQuiescing() const {
    return mQuiescingState->mNumQuiescing.load(std::memory_order_relaxed) > 0;
  }
  void Quiesce();
  absl::optional<TaskFunction> Steal();
  bool AllBlocked() const { return NumBlockedThreads() == mNumThreads; }
  void CheckCallerThread(const char* function_name) const;
  bool IsInWorkerThread() const {
    PerThread* per_thread = GetPerThread();
    return per_thread->parent == &mDerived;
  }
  void Cancel();

 private:
  template <typename ThreadingEnvironment>
  friend class BlockingWorkQueue;

  template <typename ThreadingEnvironment>
  friend class NonBlockingWorkQueue;

  struct PerThread {
    constexpr PerThread() : parent(nullptr), rng(0), thread_id(-1) {}
    Derived* parent;
    FastRng rng;    // Random number generator
    int thread_id;  // Worker thread index in the workers queue
  };

  struct ThreadData {
    ThreadData() : thread(), queue() {}
    std::unique_ptr<Thread> thread;
    Queue queue;
  };

  // Returns a TaskFunction with an attached pending tasks counter, if the
  // quiescing mode is on.
  TaskFunction WithPendingTaskCounter(TaskFunction task) {
    return TaskFunction(
        [task = std::move(task), p = PendingTask(mQuiescingState)]() mutable {
          task();
        });
  }

  // TODO(ezhulenev): Make this a runtime parameter? More spinning threads help
  // to reduce latency at the cost of wasted CPU cycles.
  static constexpr int kMaxSpinningThreads = 1;

  // The number of steal loop spin iterations before parking (this number is
  // divided by the number of threads, to get spin count for each thread).
  static constexpr int kSpinCount = 5000;

  // If there are enough active threads with an empty pending task queues, there
  // is no need for spinning before parking a thread that is out of work to do,
  // because these active threads will go into a steal loop after finishing with
  // their current tasks.
  //
  // In the worst case when all active threads are executing long/expensive
  // tasks, the next AddTask() will have to wait until one of the parked threads
  // will be unparked, however this should be very rare in practice.
  static constexpr int kMinActiveThreadsToStartSpinning = 4;

  explicit WorkQueueBase(QuiescingState* quiescing_state,
                         absl::string_view name_prefix, int num_threads);
  ~WorkQueueBase();

  // Main worker thread loop.
  void WorkerLoop(int thread_id);

  // WaitForWork() blocks until new work is available (returns true), or if it
  // is time to exit (returns false). Can optionally return a task to execute in
  // `task` (in such case `task.has_value() == true` on return).
  bool WaitForWork(EventCount::Waiter* waiter,
                   absl::optional<TaskFunction>* task);

  // StartSpinning() checks if the number of threads in the spin loop is less
  // than the allowed maximum, if so increments the number of spinning threads
  // by one and returns true (caller must enter the spin loop). Otherwise
  // returns false, and the caller must not enter the spin loop.
  bool StartSpinning();

  // StopSpinning() decrements the number of spinning threads by one. It also
  // checks if there were any tasks submitted into the pool without notifying
  // parked threads, and decrements the count by one. Returns true if the number
  // of tasks submitted without notification was decremented, in this case
  // caller thread might have to call Steal() one more time.
  bool StopSpinning();

  // IsNotifyParkedThreadRequired() returns true if parked thread must be
  // notified about new added task. If there are threads spinning in the steal
  // loop, there is no need to unpark any of the waiting threads, the task will
  // be picked up by one of the spinning threads.
  bool IsNotifyParkedThreadRequired();

  void Notify() { mEventCount.Notify(false); }

  // Returns current thread id if the caller thread is managed by `this`,
  // returns `-1` otherwise.
  int CurrentThreadId() const;

  // NonEmptyQueueIndex() returns the index of a non-empty worker queue, or `-1`
  // if all queues are empty.
  int NonEmptyQueueIndex();

  static PerThread* GetPerThread() {
    static thread_local PerThread perThread;
    PerThread* pt = &perThread;
    return pt;
  }

  unsigned NumBlockedThreads() const { return mBlocked.load(); }
  unsigned NumActiveThreads() const { return mNumThreads - mBlocked.load(); }

  const int mNumThreads;

  std::vector<ThreadData> mThreadData;
  std::vector<unsigned> mCoprimes;

  std::atomic<unsigned> mBlocked;
  std::atomic<bool> mDone;
  std::atomic<bool> mCancelled;

  // Use a conditional variable to notify waiters when all worker threads are
  // blocked. This is used to park caller thread in Quiesce() when there is no
  // work to steal, but not all threads are parked.
  std::mutex mAllBlockedMu;
  std::condition_variable mAllBlockedCV;

  // All work queues composed together in a single logical work queue, must
  // share a quiescing state to guarantee correct emptyness check.
  QuiescingState* mQuiescingState;

  // Spinning state layout:
  // - Low 32 bits encode the number of threads that are spinning in steal loop.
  //
  // - High 32 bits encode the number of tasks that were submitted to the pool
  //   without a call to mEventCount.Notify(). This number can't be larger than
  //   the number of spinning threads. Each spinning thread, when it exits the
  //   spin loop must check if this number is greater than zero, and maybe make
  //   another attempt to steal a task and decrement it by one.
  static constexpr uint64_t kNumSpinningBits = 32;
  static constexpr uint64_t kNumSpinningMask = (1ull << kNumSpinningBits) - 1;
  static constexpr uint64_t kNumNoNotifyBits = 32;
  static constexpr uint64_t kNumNoNotifyShift = 32;
  static constexpr uint64_t kNumNoNotifyMask = ((1ull << kNumNoNotifyBits) - 1)
                                               << kNumNoNotifyShift;
  std::atomic<uint64_t> mSpinningState;

  struct SpinningState {
    uint64_t mNumSpinning;        // number of spinning threads
    uint64_t mNumNoNotification;  // number of tasks submitted without
                                  // notifying waiting threads

    // Decode `mSpinningState` value.
    static SpinningState Decode(uint64_t state) {
      uint64_t mNumSpinning = (state & kNumSpinningMask);
      uint64_t mNumNoNotification =
          (state & kNumNoNotifyMask) >> kNumNoNotifyShift;

      assert(mNumNoNotification <= mNumSpinning);

      return {mNumSpinning, mNumNoNotification};
    }

    // Encode as `mSpinningState` value.
    uint64_t Encode() const {
      return (mNumNoNotification << kNumNoNotifyShift) | mNumSpinning;
    }
  };

  EventCount mEventCount;
  Derived& mDerived;
};

inline std::vector<unsigned> ComputeCoprimes(int n) {
  std::vector<unsigned> coprimes;
  for (unsigned i = 1; i <= n; i++) {
    unsigned a = i;
    unsigned b = n;
    // If GCD(a, b) == 1, then a and b are coprimes.
    while (b != 0) {
      unsigned tmp = a;
      a = b;
      b = tmp % b;
    }
    if (a == 1) coprimes.push_back(i);
  }
  return coprimes;
}

template <typename Derived>
WorkQueueBase<Derived>::WorkQueueBase(QuiescingState* quiescing_state,
                                      absl::string_view name_prefix,
                                      int num_threads)
    : mNumThreads(num_threads),
      mThreadData(num_threads),
      mCoprimes(ComputeCoprimes(num_threads)),
      mBlocked(0),
      mDone(false),
      mCancelled(false),
      mQuiescingState(quiescing_state),
      mSpinningState(0),
      mEventCount(num_threads),
      mDerived(static_cast<Derived&>(*this)) {
  assert(num_threads >= 1);
  for (int i = 0; i < num_threads; i++) {
    mThreadData[i].thread =
        ThreadingEnvironment::StartThread([this, i]() { WorkerLoop(i); });
  }
}

template <typename Derived>
WorkQueueBase<Derived>::~WorkQueueBase() {
  mDone = true;

  // Now if all threads block without work, they will start exiting.
  // But note that threads can continue to work arbitrary long,
  // block, submit new work, unblock and otherwise live full life.
  if (!mCancelled) {
    mEventCount.Notify(true);
  } else {
    // Since we were cancelled, there might be entries in the queues.
    // Empty them to prevent their destructor from asserting.
    for (ThreadData& thread_data : mThreadData) {
      thread_data.queue.Flush();
    }
  }
  // All worker threads joined in destructors.
  for (ThreadData& thread_data : mThreadData) {
    ThreadingEnvironment::Join(thread_data.thread.get());
    thread_data.thread.reset();
  }
}

template <typename Derived>
void WorkQueueBase<Derived>::CheckCallerThread(
    const char* function_name) const {
  PerThread* pt = GetPerThread();
  assert(pt->parent != this &&
         "Error, should not be called by a work thread "
         "already managed by the queue.");
  // TFRT_LOG_IF(FATAL, pt->parent == this)
  //     << "Error at " << __FILE__ << ":" << __LINE__ << ": " << function_name
  //     << " should not be called by a work thread already managed by the
  //     queue.";
}

template <typename Derived>
void WorkQueueBase<Derived>::Quiesce() {
  CheckCallerThread("WorkQueueBase::Quiesce");

  // Keep stealing tasks until we reach a point when we have nothing to steal
  // and all worker threads are in blocked state.
  absl::optional<TaskFunction> task = Steal();

  while (task.has_value()) {
    // Execute stolen task in the caller thread.
    (*task)();

    // Try to steal the next task.
    task = Steal();
  }

  // If we didn't find a task to execute, and there are still worker threads
  // running, park current thread on a conditional variable until all worker
  // threads are blocked.
  if (!AllBlocked()) {
    std::unique_lock<std::mutex> lock(mAllBlockedMu);
    mAllBlockedCV.wait(lock, [this]() { return AllBlocked(); });
  }
}

template <typename Derived>
absl::optional<TaskFunction> WorkQueueBase<Derived>::Steal() {
  PerThread* pt = GetPerThread();
  unsigned r = pt->rng();
  unsigned victim = FastReduce(r, mNumThreads);
  unsigned inc = mCoprimes[FastReduce(r, mCoprimes.size())];

  for (unsigned i = 0; i < mNumThreads; i++) {
    absl::optional<TaskFunction> t =
        mDerived.Steal(&(mThreadData[victim].queue));
    if (t.has_value()) return t;

    victim += inc;
    if (victim >= mNumThreads) {
      victim -= mNumThreads;
    }
  }
  return absl::nullopt;
}

template <typename Derived>
void WorkQueueBase<Derived>::WorkerLoop(int thread_id) {
  PerThread* pt = GetPerThread();
  pt->parent = &mDerived;
  pt->rng = FastRng(ThreadingEnvironment::ThisThreadIdHash());
  pt->thread_id = thread_id;

  Queue* q = &(mThreadData[thread_id].queue);
  EventCount::Waiter* waiter = mEventCount.waiter(thread_id);

  // TODO(dvyukov,rmlarsen): The time spent in NonEmptyQueueIndex() is
  // proportional to mNumThreads and we assume that new work is scheduled at
  // a constant rate, so we set spin_count to 5000 / mNumThreads. The
  // constant was picked based on a fair dice roll, tune it.
  const int spin_count = mNumThreads > 0 ? kSpinCount / mNumThreads : 0;

  while (!mCancelled) {
    absl::optional<TaskFunction> t = mDerived.NextTask(q);
    if (!t.has_value()) {
      t = Steal();
      if (!t.has_value()) {
        // Maybe leave thread spinning. This reduces latency.
        const bool start_spinning = StartSpinning();
        if (start_spinning) {
          for (int i = 0; i < spin_count && !t.has_value(); ++i) {
            t = Steal();
          }

          const bool stopped_spinning = StopSpinning();
          // If a task was submitted to the queue without a call to
          // `mEventCount.Notify()`, and we didn't steal anything above, we
          // must try to steal one more time, to make sure that this task will
          // be executed. We will not necessarily find it, because it might have
          // been already stolen by some other thread.
          if (stopped_spinning && !t.has_value()) {
            t = Steal();
          }
        }

        if (!t.has_value()) {
          if (!WaitForWork(waiter, &t)) {
            return;
          }
        }
      }
    }
    if (t.has_value()) {
      (*t)();  // Execute a task.
    }
  }
}

template <typename Derived>
bool WorkQueueBase<Derived>::WaitForWork(EventCount::Waiter* waiter,
                                         absl::optional<TaskFunction>* task) {
  assert(!task->has_value());
  // We already did best-effort emptiness check in Steal, so prepare for
  // blocking.
  mEventCount.Prewait();
  // Now do a reliable emptiness check.
  int victim = NonEmptyQueueIndex();
  if (victim != -1) {
    mEventCount.CancelWait();
    if (mCancelled) {
      return false;
    } else {
      *task = mDerived.Steal(&(mThreadData[victim].queue));
      return true;
    }
  }
  // Number of blocked threads is used as termination condition.
  // If we are shutting down and all worker threads blocked without work,
  // that's we are done.
  mBlocked.fetch_add(1);

  // Notify threads that are waiting for "all blocked" event.
  if (mBlocked.load() == static_cast<unsigned>(mNumThreads)) {
    std::lock_guard<std::mutex> lock(mAllBlockedMu);
    mAllBlockedCV.notify_all();
  }

  // Prepare to shutdown worker thread if done.
  if (mDone && mBlocked.load() == static_cast<unsigned>(mNumThreads)) {
    mEventCount.CancelWait();
    // Almost done, but need to re-check queues.
    // Consider that all queues are empty and all worker threads are preempted
    // right after incrementing mBlocked above. Now a free-standing thread
    // submits work and calls destructor (which sets mDone). If we don't
    // re-check queues, we will exit leaving the work unexecuted.
    if (NonEmptyQueueIndex() != -1) {
      // Note: we must not pop from queues before we decrement mBlocked,
      // otherwise the following scenario is possible. Consider that instead
      // of checking for emptiness we popped the only element from queues.
      // Now other worker threads can start exiting, which is bad if the
      // work item submits other work. So we just check emptiness here,
      // which ensures that all worker threads exit at the same time.
      mBlocked.fetch_sub(1);
      return true;
    }
    // Reached stable termination state.
    mEventCount.Notify(true);
    return false;
  }

  mEventCount.CommitWait(waiter);
  mBlocked.fetch_sub(1);
  return true;
}

template <typename Derived>
bool WorkQueueBase<Derived>::StartSpinning() {
  if (NumActiveThreads() > kMinActiveThreadsToStartSpinning) return false;

  uint64_t spinning = mSpinningState.load(std::memory_order_relaxed);
  for (;;) {
    SpinningState state = SpinningState::Decode(spinning);

    if ((state.mNumSpinning - state.mNumNoNotification) >= kMaxSpinningThreads)
      return false;

    // Increment the number of spinning threads.
    ++state.mNumSpinning;

    if (mSpinningState.compare_exchange_weak(spinning, state.Encode(),
                                             std::memory_order_relaxed)) {
      return true;
    }
  }
}

template <typename Derived>
bool WorkQueueBase<Derived>::StopSpinning() {
  uint64_t spinning = mSpinningState.load(std::memory_order_relaxed);
  for (;;) {
    SpinningState state = SpinningState::Decode(spinning);

    // Decrement the number of spinning threads.
    --state.mNumSpinning;

    // Maybe decrement the number of tasks submitted without notification.
    bool has_no_notify_task = state.mNumNoNotification > 0;
    if (has_no_notify_task) --state.mNumNoNotification;

    if (mSpinningState.compare_exchange_weak(spinning, state.Encode(),
                                             std::memory_order_relaxed)) {
      return has_no_notify_task;
    }
  }
}

template <typename Derived>
bool WorkQueueBase<Derived>::IsNotifyParkedThreadRequired() {
  uint64_t spinning = mSpinningState.load(std::memory_order_relaxed);
  for (;;) {
    SpinningState state = SpinningState::Decode(spinning);

    // If the number of tasks submitted without notifying parked threads is
    // equal to the number of spinning threads, we must wake up one of the
    // parked threads.
    if (state.mNumNoNotification == state.mNumSpinning) return true;

    // Increment the number of tasks submitted without notification.
    ++state.mNumNoNotification;

    if (mSpinningState.compare_exchange_weak(spinning, state.Encode(),
                                             std::memory_order_relaxed)) {
      return false;
    }
  }
}

template <typename Derived>
int WorkQueueBase<Derived>::NonEmptyQueueIndex() {
  PerThread* pt = GetPerThread();
  unsigned r = pt->rng();
  unsigned inc = mNumThreads == 1 ? 1 : mCoprimes[r % mCoprimes.size()];
  unsigned victim = FastReduce(r, mNumThreads);
  for (unsigned i = 0; i < mNumThreads; i++) {
    if (!mDerived.Empty(&(mThreadData[victim].queue))) {
      return static_cast<int>(victim);
    }
    victim += inc;
    if (victim >= mNumThreads) {
      victim -= mNumThreads;
    }
  }
  return -1;
}

template <typename Derived>
int WorkQueueBase<Derived>::CurrentThreadId() const {
  const PerThread* pt = GetPerThread();
  if (pt->parent == this) {
    return pt->thread_id;
  } else {
    return -1;
  }
}

template <typename Derived>
void WorkQueueBase<Derived>::Cancel() {
  mCancelled = true;
  mDone = true;

  // Wake up the threads without work to let them exit on their own.
  mEventCount.Notify(true);
}

}  // namespace internal
}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_WORK_QUEUE_BASE_ */
