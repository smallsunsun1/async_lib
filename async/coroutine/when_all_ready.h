#ifndef ASYNC_COROUTINE_WHEN_ALL_READY_
#define ASYNC_COROUTINE_WHEN_ALL_READY_

#include "async/coroutine/internal/awaiter_traits.h"
#include "async/coroutine/internal/when_all.h"

namespace sss {
namespace async {

template <typename... Awaitables, std::enable_if_t<std::conjunction_v<internal::is_awaitable<std::remove_reference_t<Awaitables>...>>, int> = 0>
auto WhenAllReady(Awaitables&&... awaitables) {
    
}

}
}  // namespace sss

#endif /* ASYNC_COROUTINE_WHEN_ALL_READY_ */
