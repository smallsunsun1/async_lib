#ifndef ASYNC_COROUTINE_INTERNAL_AWAITER_TRAITS_
#define ASYNC_COROUTINE_INTERNAL_AWAITER_TRAITS_

#include <coroutine>
#include <type_traits>
#include <utility>

#include "async/coroutine/internal/data_types.h"

namespace sss {
namespace async {
namespace internal {

template <typename T>
struct is_coroutine_handle : public std::false_type {};
template <typename T>
struct is_coroutine_handle<std::coroutine_handle<T>> : public std::true_type {};

template <typename T>
inline constexpr bool is_coroutine_handle_v = is_coroutine_handle<T>::value;

// 用于判断一个awaitable的strcut的await_suspend函数的返回值是否符合要求
template <typename T>
struct is_valid_await_suspend_return_value
    : std::disjunction<std::is_void<T>, std::is_same<T, bool>,
                       is_coroutine_handle<T>> {};

template <typename T, typename F = std::void_t<>>
struct is_awaiter : std::false_type {};

template <typename T>
struct is_awaiter<T, std::void_t<decltype(std::declval<T>().await_ready()),
                                 decltype(std::declval<T>().await_suspend()),
                                 decltype(std::declval<T>().await_resume())>>
    : public std::conjunction<
          std::is_constructible<bool,
                                decltype(std::declval<T>().await_ready())>,
          is_valid_await_suspend_return_value<
              decltype(std::declval<T>().await_suspend(
                  std::coroutine_handle<>()))>> {};

template <typename T>
inline constexpr bool is_awaiter_v = is_awaiter<T>::value;

template <typename T, std::enable_if_t<is_awaiter<T &&>::value, int> = 0>
T &&get_awaiter_impl(T &&value, AnyData) noexcept {
  return std::forward<T>(value);
}
template <typename T>
auto get_awaiter_impl(T &&value, int)
    -> decltype(std::forward<T>(value).operator co_await()) {
  return std::forward<T>(value).operator co_await();
}
template <typename T>
auto get_awaiter_impl(T &&value, long)
    -> decltype(operator co_await(std::forward<T>(value))) {
  return operator co_await(std::forward<T>(value));
}

template <typename T>
auto get_awaiter(T &&value)
    -> decltype(get_awaiter_impl(std::forward<T>(value), 1)) {
  return get_awaiter_impl(std::forward<T>(value), 1);
}

template <typename T, typename F = std::void_t<>>
struct is_awaitable : std::false_type {};

template <typename T>
struct is_awaitable<T, std::void_t<decltype(get_awaiter(std::declval<T>()))>>
    : public std::true_type {};

template <typename T>
inline constexpr bool is_awaitable_v = is_awaitable<T>::value;

template <typename T>
struct remove_rvalue_reference {
  using type = T;
};
template <typename T>
struct remove_rvalue_reference<T &&> {
  using type = T;
};

template <typename T>
using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_INTERNAL_AWAITER_TRAITS_ */
