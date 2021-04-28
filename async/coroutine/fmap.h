#ifndef ASYNC_COROUTINE_FMAP_H_
#define ASYNC_COROUTINE_FMAP_H_

#include <functional>
#include <type_traits>
#include <utility>

#include "async/coroutine/coroutine_traits.h"
#include "async/coroutine/internal/awaiter_traits.h"

namespace sss {
namespace async {
namespace internal {
template <typename Func, typename Awaitable>
class FmapAwaiter {
  using awaiter_t = typename awaitable_traits<Awaitable&&>::awaiter_t;
  Func&& mFunc;
  awaiter_t m_awaiter;

 public:
  FmapAwaiter(Func&& func, Awaitable&& awaitable) noexcept(std::is_nothrow_move_constructible_v<awaiter_t>&& noexcept(detail::get_awaiter(static_cast<Awaitable&&>(awaitable))))
      : mFunc(static_cast<Func&&>(func)), m_awaiter(detail::get_awaiter(static_cast<Awaitable&&>(awaitable))) {}

  decltype(auto) await_ready() noexcept(noexcept(static_cast<awaiter_t&&>(m_awaiter).await_ready())) { return static_cast<awaiter_t&&>(m_awaiter).await_ready(); }

  template <typename Promise>
  decltype(auto) await_suspend(std::coroutine_handle<Promise> coro) noexcept(noexcept(static_cast<awaiter_t&&>(m_awaiter).await_suspend(std::move(coro)))) {
    return static_cast<awaiter_t&&>(m_awaiter).await_suspend(std::move(coro));
  }

  template <typename AwaitResult = decltype(std::declval<awaiter_t>().await_resume()), std::enable_if_t<std::is_void_v<AwaitResult>, int> = 0>
  decltype(auto) await_resume() noexcept(noexcept(std::invoke(static_cast<Func&&>(mFunc)))) {
    static_cast<awaiter_t&&>(m_awaiter).await_resume();
    return std::invoke(static_cast<Func&&>(mFunc));
  }

  template <typename AwaitResult = decltype(std::declval<awaiter_t>().await_resume()), std::enable_if_t<!std::is_void_v<AwaitResult>, int> = 0>
  decltype(auto) await_resume() noexcept(noexcept(std::invoke(static_cast<Func&&>(mFunc), static_cast<awaiter_t&&>(m_awaiter).await_resume()))) {
    return std::invoke(static_cast<Func&&>(mFunc), static_cast<awaiter_t&&>(m_awaiter).await_resume());
  }
};

template <typename Func, typename Awaitable>
class fmap_awaitable {
  static_assert(!std::is_lvalue_reference_v<Func>);
  static_assert(!std::is_lvalue_reference_v<Awaitable>);

 public:
  template <typename FuncArg, typename AwaitableArg, std::enable_if_t<std::is_constructible_v<Func, FuncArg&&> && std::is_constructible_v<Awaitable, AwaitableArg&&>, int> = 0>
  explicit fmap_awaitable(FuncArg&& func, AwaitableArg&& awaitable) noexcept(std::is_nothrow_constructible_v<Func, FuncArg&&>&& std::is_nothrow_constructible_v<Awaitable, AwaitableArg&&>)
      : mFunc(static_cast<FuncArg&&>(func)), mAwaitable(static_cast<AwaitableArg&&>(awaitable)) {}

  auto operator co_await() const& { return FmapAwaiter<const Func&, const Awaitable&>(mFunc, mAwaitable); }

  auto operator co_await() & { return FmapAwaiter<Func&, Awaitable&>(mFunc, mAwaitable); }

  auto operator co_await() && { return FmapAwaiter<Func&&, Awaitable&&>(static_cast<Func&&>(mFunc), static_cast<Awaitable&&>(mAwaitable)); }

 private:
  Func mFunc;
  Awaitable mAwaitable;
};
}  // namespace internal
template <typename Func>
struct FmapTransform {
  explicit FmapTransform(Func&& f) noexcept(std::is_nothrow_move_constructible_v<Func>) : func(std::forward<Func>(f)) {}

  Func func;
};

template <typename Func, typename Awaitable, std::enable_if_t<internal::is_awaitable_v<Awaitable>, int> = 0>
auto Fmap(Func&& func, Awaitable&& awaitable) {
  return detail::fmap_awaitable<std::remove_cv_t<std::remove_reference_t<Func>>, std::remove_cv_t<std::remove_reference_t<Awaitable>>>(std::forward<Func>(func), std::forward<Awaitable>(awaitable));
}

template <typename Func>
auto Fmap(Func&& func) {
  return FmapTransform<Func>{std::forward<Func>(func)};
}

template <typename T, typename Func>
decltype(auto) operator|(T&& value, FmapTransform<Func>&& transform) {
  // Use ADL for finding Fmap() overload.
  return Fmap(std::forward<Func>(transform.func), std::forward<T>(value));
}

template <typename T, typename Func>
decltype(auto) operator|(T&& value, const FmapTransform<Func>& transform) {
  // Use ADL for finding Fmap() overload.
  return Fmap(transform.func, std::forward<T>(value));
}

template <typename T, typename Func>
decltype(auto) operator|(T&& value, FmapTransform<Func>& transform) {
  // Use ADL for finding Fmap() overload.
  return Fmap(transform.func, std::forward<T>(value));
}

}  // namespace async
}  // namespace sss

#endif  // ASYNC_COROUTINE_FMAP_H_
