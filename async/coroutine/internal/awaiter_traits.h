#ifndef ASYNC_COROUTINE_INTERNAL_AWAITER_TRAITS_
#define ASYNC_COROUTINE_INTERNAL_AWAITER_TRAITS_

#include <coroutine>
#include <type_traits>

#include "data_types.h"

namespace sss {
namespace async {
namespace internal {

template <typename T>
struct is_coroutine_handle : public std::false_type {};
template <typename T>
struct is_coroutine_handle<std::coroutine_handle<T>> : public std::true_type {};

// 用于判断一个awaitable的strcut的await_suspend函数的返回值是否符合要求
template <typename T>
struct is_valid_await_suspend_return_value : std::disjunction<std::is_void<T>, std::is_same<T, bool>, is_coroutine_handle<T>> {};

template <typename T, typename F = std::void_t<>>
struct is_awaiter : std::false_type {};

template <typename T>
struct is_awaiter<T, std::void_t<decltype(std::declval<T>().await_ready()), decltype(std::declval<T>().await_suspend()), decltype(std::declval<T>().await_resume())>>
    : public std::conjunction<std::is_constructible<bool, decltype(std::declval<T>().await_ready())>,
                              is_valid_await_suspend_return_value<decltype(std::declval<T>().await_suspend(std::coroutine_handle<>()))>> {};

template <typename T, std::enable_if_t<is_awaiter<T&&>::value, int> = 0>
T&& get_awaiter_impl(T&& value, AnyData) noexcept {
  return std::forward<T>(value);
}
template <typename T>
auto get_awaiter_impl(T&& value, int) -> decltype(std::forward<T>(value).operator co_await()) {
  return std::forward<T>(value).operator co_await();
}
template <typename T>
auto get_awaiter_impl(T&& value, long) -> decltype(operator co_await(std::forward<T>(value))) {
  return operator co_await(std::forward<T>(value));
}

template <typename T>
auto get_awaiter(T&& value) -> decltype(get_awaiter_impl(std::forward<T>(value), 1)) {
  return get_awaiter_impl(std::forward<T>(value), 1);
}

template <typename T, typename F = std::void_t<>>
struct is_awaitable : std::false_type {};

template <typename T>
struct is_awaitbale<T, std::void_t<decltype(get_awaiter(std::declval<T>()))>> : public std::true_type {};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_INTERNAL_AWAITER_TRAITS_ */
