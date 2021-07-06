#ifndef ASYNC_COROUTINE_INTERNAL_WHEN_ALL_READY_AWAITABLE_H_
#define ASYNC_COROUTINE_INTERNAL_WHEN_ALL_READY_AWAITABLE_H_

#include <coroutine>
#include <tuple>

#include "async/coroutine/internal/when_all_counter.h"

namespace sss {
namespace async {
namespace internal {
template <typename TaskContainer>
class WhenAllReadyAwaitable;

template <>
class WhenAllReadyAwaitable<std::tuple<>> {
 public:
  constexpr WhenAllReadyAwaitable() noexcept {}
  explicit constexpr WhenAllReadyAwaitable(std::tuple<>) noexcept {}

  constexpr bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  std::tuple<> await_resume() const noexcept { return {}; }
};

template <typename... Tasks>
class WhenAllReadyAwaitable<std::tuple<Tasks...>> {
 public:
  explicit WhenAllReadyAwaitable(Tasks &&...tasks) noexcept(
      std::conjunction_v<std::is_nothrow_move_constructible<Tasks>...>)
      : mCounter(sizeof...(Tasks)), mTasks(std::move(tasks)...) {}

  explicit WhenAllReadyAwaitable(std::tuple<Tasks...> &&tasks) noexcept(
      std::is_nothrow_move_constructible_v<std::tuple<Tasks...>>)
      : mCounter(sizeof...(Tasks)), mTasks(std::move(tasks)) {}

  WhenAllReadyAwaitable(WhenAllReadyAwaitable &&other) noexcept
      : mCounter(sizeof...(Tasks)), mTasks(std::move(other.mTasks)) {}

  auto operator co_await() &noexcept {
    struct awaiter {
      awaiter(WhenAllReadyAwaitable &awaitable) noexcept
          : m_awaitable(awaitable) {}

      bool await_ready() const noexcept { return m_awaitable.is_ready(); }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      std::tuple<Tasks...> &await_resume() noexcept {
        return m_awaitable.mTasks;
      }

     private:
      WhenAllReadyAwaitable &m_awaitable;
    };

    return awaiter{*this};
  }

  auto operator co_await() &&noexcept {
    struct awaiter {
      awaiter(WhenAllReadyAwaitable &awaitable) noexcept
          : m_awaitable(awaitable) {}

      bool await_ready() const noexcept { return m_awaitable.is_ready(); }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      std::tuple<Tasks...> &&await_resume() noexcept {
        return std::move(m_awaitable.mTasks);
      }

     private:
      WhenAllReadyAwaitable &m_awaitable;
    };

    return awaiter{*this};
  }

 private:
  bool is_ready() const noexcept { return mCounter.is_ready(); }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept {
    start_tasks(std::make_integer_sequence<std::size_t, sizeof...(Tasks)>{});
    return mCounter.try_await(awaitingCoroutine);
  }

  template <std::size_t... INDICES>
  void start_tasks(std::integer_sequence<std::size_t, INDICES...>) noexcept {
    (void)std::initializer_list<int>{
        (std::get<INDICES>(mTasks).start(mCounter), 0)...};
  }

  WhenAllCounter mCounter;
  std::tuple<Tasks...> mTasks;
};

template <typename TaskContainer>
class WhenAllReadyAwaitable {
 public:
  explicit WhenAllReadyAwaitable(TaskContainer &&tasks) noexcept
      : mCounter(tasks.size()), mTasks(std::forward<TaskContainer>(tasks)) {}

  WhenAllReadyAwaitable(WhenAllReadyAwaitable &&other) noexcept(
      std::is_nothrow_move_constructible_v<TaskContainer>)
      : mCounter(other.mTasks.size()), mTasks(std::move(other.mTasks)) {}

  WhenAllReadyAwaitable(const WhenAllReadyAwaitable &) = delete;
  WhenAllReadyAwaitable &operator=(const WhenAllReadyAwaitable &) = delete;

  auto operator co_await() &noexcept {
    class awaiter {
     public:
      awaiter(WhenAllReadyAwaitable &awaitable) : m_awaitable(awaitable) {}

      bool await_ready() const noexcept { return m_awaitable.is_ready(); }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      TaskContainer &await_resume() noexcept { return m_awaitable.mTasks; }

     private:
      WhenAllReadyAwaitable &m_awaitable;
    };

    return awaiter{*this};
  }

  auto operator co_await() &&noexcept {
    class awaiter {
     public:
      awaiter(WhenAllReadyAwaitable &awaitable) : m_awaitable(awaitable) {}

      bool await_ready() const noexcept { return m_awaitable.is_ready(); }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      TaskContainer &&await_resume() noexcept {
        return std::move(m_awaitable.mTasks);
      }

     private:
      WhenAllReadyAwaitable &m_awaitable;
    };

    return awaiter{*this};
  }

 private:
  bool is_ready() const noexcept { return mCounter.is_ready(); }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept {
    for (auto &&task : mTasks) {
      task.start(mCounter);
    }

    return mCounter.try_await(awaitingCoroutine);
  }

  WhenAllCounter mCounter;
  TaskContainer mTasks;
};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif  // ASYNC_COROUTINE_INTERNAL_WHEN_ALL_READY_AWAITABLE_H_
