#ifndef ASYNC_COROUTINE_WHEN_ALL_READY_
#define ASYNC_COROUTINE_WHEN_ALL_READY_

#include <vector>

#include "async/coroutine/coroutine_traits.h"
#include "async/coroutine/internal/awaiter_traits.h"
#include "async/coroutine/internal/when_all.h"

namespace sss {
namespace async {

template <typename... Awaitables, std::enable_if_t<std::conjunction_v<internal::is_awaitable<std::remove_reference_t<Awaitables>>...>, int> = 0>
auto WhenAllReady(Awaitables&&... awaitables) {
  return internal::WhenAllAwaitable<std::tuple<internal::WhenAllTasks<typename awaitable_traits<std::remove_reference_t<Awaitables>>::await_result_t>...>>(
      std::make_tuple(internal::make_when_all_tasks(std::forward<Awaitables>(awaitables))...));
}

template <typename Awaitable, typename Result = typename awaitable_traits<std::unwrap_reference_t<Awaitable>>::await_result_t>
auto WhenAllReady(std::vector<Awaitable> awaitables) {
  std::vector<internal::WhenAllTasks<Result>> tasks;
  tasks.reserve(awaitables.size());
  for (auto& awaitable : awaitables) {
    tasks.emplace_back(internal::make_when_all_tasks(std::move(awaitable)));
  }
  return internal::WhenAllAwaitable<std::vector<internal::WhenAllTasks<Result>>>(std::move(tasks));
}

template <typename... Awaitables, std::enable_if_t<std::conjunction_v<internal::is_awaitable<std::unwrap_reference_t<std::remove_reference_t<Awaitables>>>...>, int> = 0>
auto WhenAll(Awaitables&&... awaitables) {
  return FMap(
      [](auto&& taskTuple) {
        return std::apply([](auto&&... tasks) { return std::make_tuple(static_cast<decltype(tasks)>(tasks).non_void_result()...); }, static_cast<decltype(taskTuple)>(taskTuple));
      },
      WhenAllReady(std::forward<Awaitables>(awaitables)...));
}

template <typename Awaitable, typename Result = typename awaitable_traits<std::unwrap_reference_t<Awaitable>>::await_result_t, std::enable_if_t<std::is_void_v<Result>, int> = 0>
auto WhenAll(std::vector<Awaitable> awaitables) {
  return FMap(
      [](auto&& taskVector) {
        for (auto& task : taskVector) {
          task.result();
        }
      },
      WhenAllReady(std::move(awaitables)));
}

template <typename Awaitable, typename Result = typename awaitable_traits<std::unwrap_reference_t<Awaitable>>::await_result_t, std::enable_if_t<!std::is_void_v<Result>, int> = 0>
auto WhenAll(std::vector<Awaitable> awaitables) {
  using result_t = std::conditional_t<std::is_lvalue_reference_v<Result>, std::reference_wrapper<std::remove_reference_t<Result>>, std::remove_reference_t<Result>>;
  return FMap(
      [](auto&& taskVector) {
        std::vector<result_t> results;
        results.reserve(taskVector.size());
        for (auto& task : taskVector) {
          if constexpr (std::is_rvalue_reference_v<decltype(taskVector)>) {
            results.emplace_back(std::move(task).result());
          } else {
            results.emplace_back(task.result());
          }
        }
        return results;
      },
      WhenAllReady(std::move(awaitables)));
}

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_WHEN_ALL_READY_ */
