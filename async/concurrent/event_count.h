#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_EVENT_COUNT_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_EVENT_COUNT_

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace ficus {
namespace async {
namespace internal {
class EventCount {
 public:
  struct Waiter;

  explicit EventCount(unsigned num_waiters)
      : mState(kStackMask), mWaiters(num_waiters) {
    assert(num_waiters < (1u << kCounterBits) - 1);
  }

  EventCount(const EventCount&) = delete;
  void operator=(const EventCount&) = delete;

  ~EventCount() {
    // Ensure there are no waiters.
    assert(mState.load() == kStackMask);
  }

  Waiter* waiter(unsigned index) { return &mWaiters[index]; }

  // Prewait prepares for waiting.
  // After calling Prewait, the thread must re-check the wait predicate
  // and then call either CancelWait or CommitWait.
  void Prewait() {
    uint64_t state = mState.load(std::memory_order_relaxed);
    for (;;) {
      CheckState(state);
      uint64_t newstate = state + kWaiterInc;
      CheckState(newstate);
      if (mState.compare_exchange_weak(state, newstate,
                                       std::memory_order_seq_cst))
        return;
    }
  }

  // CommitWait commits waiting after Prewait.
  void CommitWait(Waiter* w) {
    assert((w->epoch & ~kEpochMask) == 0);
    w->state = Waiter::kNotSignaled;
    const uint64_t me = (w - &mWaiters[0]) | w->epoch;
    uint64_t state = mState.load(std::memory_order_seq_cst);
    for (;;) {
      CheckState(state, true);
      uint64_t newstate;
      if ((state & kSignalMask) != 0) {
        // Consume the signal and return immidiately.
        newstate = state - kWaiterInc - kSignalInc;
      } else {
        // Remove this thread from pre-wait counter and add to the waiter stack.
        newstate = ((state & kWaiterMask) - kWaiterInc) | me;
        w->next.store(state & (kStackMask | kEpochMask),
                      std::memory_order_relaxed);
      }
      CheckState(newstate);
      if (mState.compare_exchange_weak(state, newstate,
                                       std::memory_order_acq_rel)) {
        if ((state & kSignalMask) == 0) {
          w->epoch += kEpochInc;
          Park(w);
        }
        return;
      }
    }
  }

  // CancelWait cancels effects of the previous Prewait call.
  void CancelWait() {
    uint64_t state = mState.load(std::memory_order_relaxed);
    for (;;) {
      CheckState(state, true);
      uint64_t newstate = state - kWaiterInc;
      // We don't know if the thread was also notified or not,
      // so we should not consume a signal unconditionaly.
      // Only if number of waiters is equal to number of signals,
      // we know that the thread was notified and we must take away the signal.
      if (((state & kWaiterMask) >> kWaiterShift) ==
          ((state & kSignalMask) >> kSignalShift))
        newstate -= kSignalInc;
      CheckState(newstate);
      if (mState.compare_exchange_weak(state, newstate,
                                       std::memory_order_acq_rel))
        return;
    }
  }

  // Notify wakes one or all waiting threads.
  // Must be called after changing the associated wait predicate.
  void Notify(bool notify_all) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    uint64_t state = mState.load(std::memory_order_acquire);
    for (;;) {
      CheckState(state);
      const uint64_t waiters = (state & kWaiterMask) >> kWaiterShift;
      const uint64_t signals = (state & kSignalMask) >> kSignalShift;
      // Easy case: no waiters.
      if ((state & kStackMask) == kStackMask && waiters == signals) return;
      uint64_t newstate;
      if (notify_all) {
        // Empty wait stack and set signal to number of pre-wait threads.
        newstate =
            (state & kWaiterMask) | (waiters << kSignalShift) | kStackMask;
      } else if (signals < waiters) {
        // There is a thread in pre-wait state, unblock it.
        newstate = state + kSignalInc;
      } else {
        // Pop a waiter from list and unpark it.
        Waiter* w = &mWaiters[state & kStackMask];
        uint64_t next = w->next.load(std::memory_order_relaxed);
        newstate = (state & (kWaiterMask | kSignalMask)) | next;
      }
      CheckState(newstate);
      if (mState.compare_exchange_weak(state, newstate,
                                       std::memory_order_acq_rel)) {
        if (!notify_all && (signals < waiters))
          return;  // unblocked pre-wait thread
        if ((state & kStackMask) == kStackMask) return;
        Waiter* w = &mWaiters[state & kStackMask];
        if (!notify_all) w->next.store(kStackMask, std::memory_order_relaxed);
        Unpark(w);
        return;
      }
    }
  }

  struct Waiter {
    friend class EventCount;
    // Align to 128 byte boundary to prevent false sharing with other Waiter
    // objects in the same vector.
    alignas(128) std::atomic<uint64_t> next;
    std::mutex mu;
    std::condition_variable cv;
    uint64_t epoch = 0;
    unsigned state = kNotSignaled;
    enum {
      kNotSignaled,
      kWaiting,
      kSignaled,
    };
  };

 private:
  // We use 14 bits for counting waiters, which puts a 2^14-1 (16383) limit on
  // the maximum number of waiters (threads).
  static constexpr uint64_t kCounterBits = 14;
  static constexpr uint64_t kCounterMask = (1ull << kCounterBits) - 1;

  // State layout:
  // - low kCounterBits points to the top of the stack of parked waiters (index
  //   in a `mWaiters` array). kStackMask means an empty stack.
  // - next kCounterBits is count of waiters in prewait state.
  // - next kCounterBits is count of pending signals.
  // - remaining bits are ABA counter for the stack.
  //   (stored in Waiter node and incremented on push).
  static constexpr uint64_t kStackMask = kCounterMask;

  static constexpr uint64_t kWaiterShift = kCounterBits;
  static constexpr uint64_t kWaiterMask = kCounterMask << kWaiterShift;
  static constexpr uint64_t kWaiterInc = 1ull << kWaiterShift;

  static constexpr uint64_t kSignalShift = 2 * kCounterBits;
  static constexpr uint64_t kSignalMask = kCounterMask << kSignalShift;
  static constexpr uint64_t kSignalInc = 1ull << kSignalShift;

  static constexpr uint64_t kEpochShift = 3 * kCounterBits;
  static constexpr uint64_t kEpochBits = 64 - kEpochShift;
  static constexpr uint64_t kEpochMask = ((1ull << kEpochBits) - 1)
                                         << kEpochShift;
  static constexpr uint64_t kEpochInc = 1ull << kEpochShift;

  std::atomic<uint64_t> mState;
  std::vector<Waiter> mWaiters;

  void Park(Waiter* w) {
    std::unique_lock<std::mutex> lock(w->mu);
    while (w->state != Waiter::kSignaled) {
      w->state = Waiter::kWaiting;
      w->cv.wait(lock);
    }
  }

  void Unpark(Waiter* w) {
    for (Waiter* next; w; w = next) {
      uint64_t wnext = w->next.load(std::memory_order_relaxed) & kStackMask;
      next = wnext == kStackMask ? nullptr : &mWaiters[wnext];
      unsigned state;
      {
        std::lock_guard<std::mutex> lock(w->mu);
        state = w->state;
        w->state = Waiter::kSignaled;
      }
      // Avoid notifying if it wasn't waiting.
      if (state == Waiter::kWaiting) w->cv.notify_one();
    }
  }

  static void CheckState(uint64_t state, bool waiter = false) {
    static_assert(kEpochBits >= 20, "not enough bits to prevent ABA problem");
    const uint64_t waiters = (state & kWaiterMask) >> kWaiterShift;
    const uint64_t signals = (state & kSignalMask) >> kSignalShift;
    assert(waiters >= signals);
    assert(waiters < (1u << kCounterBits) - 1);
    assert(!waiter || waiters > 0);
    (void)waiters;
    (void)signals;
  }
};

}  // namespace internal
}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_EVENT_COUNT_ */
