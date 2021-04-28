#ifndef ASYNC_COROUTINE_INTERNAL_WHEN_ALL_TASK_H_
#define ASYNC_COROUTINE_INTERNAL_WHEN_ALL_TASK_H_

#include <cassert>
#include <coroutine>

#include "async/coroutine/internal/awaiter_traits.h"
#include "async/coroutine/internal/void_value.h"
#include "async/coroutine/internal/WhenAllCounter.h"

namespace sss {
namespace async {
namespace internal {

template <typename TaskContainer>
class WhenAllReadyAwaitable;

template <typename Result>
class WhenAllTask;

template <typename Result>
class WhenAllTaskPromise final {
 public:
  using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<Result>>;

  WhenAllTaskPromise() noexcept {}

  auto get_return_object() noexcept { return coroutine_handle_t::from_promise(*this); }

  std::suspend_always initial_suspend() noexcept { return {}; }

  auto final_suspend() noexcept {
    class completion_notifier {
     public:
      bool await_ready() const noexcept { return false; }

      void await_suspend(coroutine_handle_t coro) const noexcept { coro.promise().mCounter->notify_awaitable_completed(); }

      void await_resume() const noexcept {}
    };

    return completion_notifier{};
  }

  void unhandled_exception() noexcept { mExceptionPtr = std::current_exception(); }

  void return_void() noexcept {
    // We should have either suspended at co_yield point or
    // an exception was thrown before running off the end of
    // the coroutine.
    assert(false);
  }

  auto yield_value(Result&& result) noexcept {
    m_result = std::addressof(result);
    return final_suspend();
  }

  void start(WhenAllCounter& counter) noexcept {
    mCounter = &counter;
    coroutine_handle_t::from_promise(*this).resume();
  }

  Result& result() & {
    rethrow_if_exception();
    return *m_result;
  }

  Result&& result() && {
    rethrow_if_exception();
    return std::forward<Result>(*m_result);
  }

 private:
  void rethrow_if_exception() {
    if (mExceptionPtr) {
      std::rethrow_exception(mExceptionPtr);
    }
  }

  WhenAllCounter* mCounter;
  std::exception_ptr mExceptionPtr;
  std::add_pointer_t<Result> m_result;
};

template <>
class WhenAllTaskPromise<void> final {
 public:
  using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<void>>;

  WhenAllTaskPromise() noexcept {}

  auto get_return_object() noexcept { return coroutine_handle_t::from_promise(*this); }

  std::suspend_always initial_suspend() noexcept { return {}; }

  auto final_suspend() noexcept {
    class completion_notifier {
     public:
      bool await_ready() const noexcept { return false; }

      void await_suspend(coroutine_handle_t coro) const noexcept { coro.promise().mCounter->notify_awaitable_completed(); }

      void await_resume() const noexcept {}
    };

    return completion_notifier{};
  }

  void unhandled_exception() noexcept { mExceptionPtr = std::current_exception(); }

  void return_void() noexcept {}

  void start(WhenAllCounter& counter) noexcept {
    mCounter = &counter;
    coroutine_handle_t::from_promise(*this).resume();
  }

  void result() {
    if (mExceptionPtr) {
      std::rethrow_exception(mExceptionPtr);
    }
  }

 private:
  WhenAllCounter* mCounter;
  std::exception_ptr mExceptionPtr;
};

template <typename Result>
class WhenAllTask final {
 public:
  using promise_type = WhenAllTaskPromise<Result>;

  using coroutine_handle_t = typename promise_type::coroutine_handle_t;

  WhenAllTask(coroutine_handle_t coroutine) noexcept : mCoroutine(coroutine) {}

  WhenAllTask(WhenAllTask&& other) noexcept : mCoroutine(std::exchange(other.mCoroutine, coroutine_handle_t{})) {}

  ~WhenAllTask() {
    if (mCoroutine) mCoroutine.destroy();
  }

  WhenAllTask(const WhenAllTask&) = delete;
  WhenAllTask& operator=(const WhenAllTask&) = delete;

  decltype(auto) result() & { return mCoroutine.promise().result(); }

  decltype(auto) result() && { return std::move(mCoroutine.promise()).result(); }

  decltype(auto) non_void_result() & {
    if constexpr (std::is_void_v<decltype(this->result())>) {
      this->result();
      return void_value{};
    } else {
      return this->result();
    }
  }

  decltype(auto) non_void_result() && {
    if constexpr (std::is_void_v<decltype(this->result())>) {
      std::move(*this).result();
      return void_value{};
    } else {
      return std::move(*this).result();
    }
  }

 private:
  template <typename TaskContainer>
  friend class WhenAllReadyAwaitable;

  void start(WhenAllCounter& counter) noexcept { mCoroutine.promise().start(counter); }

  coroutine_handle_t mCoroutine;
};

template <typename Awaitable, typename Result = typename awaitable_traits<Awaitable&&>::await_result_t, std::enable_if_t<!std::is_void_v<Result>, int> = 0>
WhenAllTask<Result> MakeWhenAllTask(Awaitable awaitable) {
  co_yield co_await static_cast<Awaitable&&>(awaitable);
}

template <typename Awaitable, typename Result = typename awaitable_traits<Awaitable&&>::await_result_t, std::enable_if_t<std::is_void_v<Result>, int> = 0>
WhenAllTask<void> MakeWhenAllTask(Awaitable awaitable) {
  co_await static_cast<Awaitable&&>(awaitable);
}

template <typename Awaitable, typename Result = typename awaitable_traits<Awaitable&>::await_result_t, std::enable_if_t<!std::is_void_v<Result>, int> = 0>
WhenAllTask<Result> MakeWhenAllTask(std::reference_wrapper<Awaitable> awaitable) {
  co_yield co_await awaitable.get();
}

template <typename Awaitable, typename Result = typename awaitable_traits<Awaitable&>::await_result_t, std::enable_if_t<std::is_void_v<Result>, int> = 0>
WhenAllTask<void> MakeWhenAllTask(std::reference_wrapper<Awaitable> awaitable) {
  co_await awaitable.get();
}

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif  // ASYNC_COROUTINE_INTERNAL_WHEN_ALL_TASK_H_
