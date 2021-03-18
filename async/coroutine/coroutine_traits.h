#ifndef ASYNC_COROUTINE_COROUTINE_TRAITS_
#define ASYNC_COROUTINE_COROUTINE_TRAITS_

#include "internal/awaiter_traits.h"
namespace sss {
namespace async {

template <typename T, typename = void>
struct awaitable_traits {};
template <typename T>
struct awaitable_traits<T, std::void_t<decltype(internal::get_awaiter(std::declval<T>()))>> {
    using awaiter_t = decltype(internal::get_awaiter(std::declval<T>()));
    using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
};

}
}

#endif /* ASYNC_COROUTINE_COROUTINE_TRAITS_ */
