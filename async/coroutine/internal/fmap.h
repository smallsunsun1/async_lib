#ifndef ASYNC_COROUTINE_INTERNAL_FMAP_
#define ASYNC_COROUTINE_INTERNAL_FMAP_

#include <functional>

#include "async/coroutine/coroutine_traits.h"
#include "awaiter_traits.h"

namespace sss {
namespace async {
namespace internal {

template <typename Func, typename Awaiter>
class FunctionMapAwaiter {
  using awaiter_t = typename awaitable_traits<Awaiter&&>::awaiter_t;

 public:
  FunctionMapAwaiter(Func&& func, Awaiter&& awaiter) : mFunc(std::forward<Func>(func)), mAwaiter(get_awaiter(std::forward<Awaiter>(awaiter))) {}
  bool await_ready() const { return mAwaiter.await_ready(); }
  template <typename Promise>
  decltype(auto) await_suspend(std::coroutine_handle<Promise> h) {
    return static_cast<awaiter_t&&>(mAwaiter).await_suspend(std::move(h));
  }
  template <typename AwaitResult = decltype(std::declval<awaiter_t>().await_resume()), std::enable_if_t<!std::is_void_v<AwaitResult>, int> = 0>
  decltype(auto) await_resume() {
    return std::invoke(std::forward<Func>(mFunc), mAwaiter.await_resume());
  }
  template <typename AwaitResult = decltype(std::declval<awaiter_t>().await_resume()), std::enable_if_t<std::is_void_v<AwaitResult>, int> = 0>
  decltype(auto) await_resume() {
    static_cast<awaiter_t&&>(mAwaiter).await_resume();
    std::invoke(std::forward<Func>(mFunc));
  }

 private:
  Func&& mFunc;
  awaiter_t mAwaiter;
};

template <typename Func, typename Awaitable>
class FunctionMapAwaitable {
  static_assert(!std::is_lvalue_reference_v<Func>);
  static_assert(!std::is_lvalue_reference_v<Awaitable>);

 public:
  template <typename FuncArg, typename AwaitbaleArg, std::enable_if_t<std::is_constructible_v<Func, FuncArg&&> && std::is_constructible_v<Awaitable, AwaitbaleArg&&>, int> = 0>
  explicit FunctionMapAwaitable(FuncArg&& func, AwaitbaleArg&& awaitable) : mFunc(static_cast<FuncArg&&>(func)), mAwaitbale(static_cast<AwaitbaleArg&&>(awaitable)) {}
  auto operator co_await() const& { return FunctionMapAwaiter<const Func&, const Awaitable&>(mFunc, mAwaitbale); }
  auto operator co_await() & { return FunctionMapAwaiter<Func&, Awaitable&>(mFunc, mAwaitbale); }
  auto operator co_await() && { return FunctionMapAwaiter<Func&&, Awaitable&&>(static_cast<Func&&>(mFunc), static_cast<Awaitable&&>(mAwaitbale)); }

 private:
  Func mFunc;
  Awaitable mAwaitbale;
};

template <typename Func>
struct FunctionMapTransform {
  explicit FunctionMapTransform(Func&& f) noexcept(std::is_nothrow_move_constructible_v<Func>) : mFunc(std::forward<Func>(f)) {}
  Func mFunc;
};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_INTERNAL_FMAP_ */
