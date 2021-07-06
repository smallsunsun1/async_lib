#ifndef ASYNC_COROUTINE_INTERNAL_WHEN_ALL_COUNTER_H_
#define ASYNC_COROUTINE_INTERNAL_WHEN_ALL_COUNTER_H_

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <cstdint>

namespace sss {
namespace async {
namespace internal {
class WhenAllCounter {
 public:
  WhenAllCounter(size_t count) noexcept
      : m_count(count + 1), mAwaitingCoroutine(nullptr) {}

  bool is_ready() const noexcept {
    // We consider this complete if we're asking whether it's ready
    // after a coroutine has already been registered.
    return static_cast<bool>(mAwaitingCoroutine);
  }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept {
    mAwaitingCoroutine = awaitingCoroutine;
    return m_count.fetch_sub(1, std::memory_order_acq_rel) > 1;
  }

  void notify_awaitable_completed() noexcept {
    if (m_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      mAwaitingCoroutine.resume();
    }
  }

 protected:
  std::atomic<size_t> m_count;
  std::coroutine_handle<> mAwaitingCoroutine;
};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif  // ASYNC_COROUTINE_INTERNAL_WHEN_ALL_COUNTER_H_
