#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_LATCH_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_LATCH_

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace ficus {
namespace async {

class latch {
 public:
  explicit latch(std::ptrdiff_t count)
      : state_(static_cast<uint64_t>(count) << 1), notified_(false) {
    assert(count >= 0);
    assert(static_cast<uint64_t>(count) < (1ull << 63));
  }

  ~latch() { assert(try_wait()); }

  latch(const latch&) = delete;
  latch& operator=(const latch&) = delete;

  // Decrements the counter by `n`.
  void count_down(std::ptrdiff_t n = 1);
  // Increments the counter by `n`
  void add_count(std::ptrdiff_t n = 1);

  // Returns true if the internal counter equals zero.
  bool try_wait() const noexcept;
  // Blocks until the counter reaches zero.
  void wait() const;
  // Decrements the counter and blocks until it reaches zero.
  void arrive_and_wait(std::ptrdiff_t n = 1);

 private:
  mutable std::mutex mu_;
  mutable std::condition_variable cv_;
  mutable std::atomic<uint64_t> state_;
  bool notified_;
};

inline void latch::add_count(std::ptrdiff_t n) {
  assert(n >= 0);
  state_.fetch_add(n * 2);
}

inline void latch::count_down(std::ptrdiff_t n) {
  assert(n >= 0);
  uint64_t state = state_.fetch_sub(n * 2);
  assert((state >> 1) >= n);
  if ((state >> 1) == n && (state & 1) == 1) {
    std::lock_guard<std::mutex> lock(mu_);
    cv_.notify_all();
    assert(!notified_);
    notified_ = true;
  }
}
inline bool latch::try_wait() const noexcept {
  uint64_t state = state_.load();
  return (state >> 1) == 0;
}
inline void latch::wait() const {
  uint64_t state = state_.fetch_or(1);
  // Counter already dropped reaches zero
  if ((state >> 1) == 0) return;
  std::unique_lock<std::mutex> lock(mu_);
  cv_.wait(lock, [this]() { return notified_; });
}
inline void latch::arrive_and_wait(std::ptrdiff_t n) {
  count_down(n);
  wait();
}

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_LATCH_ */
