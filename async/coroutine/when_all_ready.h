#ifndef ASYNC_COROUTINE_WHEN_ALL_READY_H_
#define ASYNC_COROUTINE_WHEN_ALL_READY_H_

#include <tuple>
#include <type_traits>
#include <vector>

#include "async/coroutine/internal/awaiter_traits.h"
#include "async/coroutine/internal/when_all_ready_awaitable.h"
#include "async/coroutine/internal/when_all_task.h"

namespace sss {
namespace async {

template <typename... Awaitables,
          std::enable_if_t<
              std::conjunction_v<internal::is_awaitable<std::unwrap_reference_t<
                  std::remove_reference_t<Awaitables>>>...>,
              int> = 0>
[[nodiscard]] auto WhenAllReady(Awaitables &&...awaitables) {
  return internal::WhenAllReadyAwaitable<std::tuple<
      internal::WhenAllTask<typename awaitable_traits<std::unwrap_reference_t<
          std::remove_reference_t<Awaitables>>>::await_result_t>...>>(
      std::make_tuple(
          internal::MakeWhenAllTask(std::forward<Awaitables>(awaitables))...));
}

template <typename Awaitable,
          typename Result = typename awaitable_traits<
              std::unwrap_reference_t<Awaitable>>::await_result_t>
[[nodiscard]] auto WhenAllReady(std::vector<Awaitable> awaitables) {
  std::vector<internal::WhenAllTask<Result>> tasks;

  tasks.reserve(awaitables.size());

  for (auto &awaitable : awaitables) {
    tasks.emplace_back(internal::MakeWhenAllTask(std::move(awaitable)));
  }

  return internal::WhenAllReadyAwaitable<
      std::vector<internal::WhenAllTask<Result>>>(std::move(tasks));
}

}  // namespace async
}  // namespace sss

#endif  // ASYNC_COROUTINE_WHEN_ALL_READY_H_
