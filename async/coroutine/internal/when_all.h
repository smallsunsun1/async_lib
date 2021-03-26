#ifndef ASYNC_COROUTINE_INTERNAL_WHEN_ALL_
#define ASYNC_COROUTINE_INTERNAL_WHEN_ALL_

#include <atomic>
#include <coroutine>
#include <exception>
#include <stdexcept>
#include <tuple>

namespace sss {
namespace async {
namespace internal {

class WhenAllCounter {
 public:
  WhenAllCounter(int count) noexcept : mCount(count + 1) {}
  bool IsReady() { return mCount.load() == 0; }
  bool TryAwait(std::coroutine_handle<> awaitCoroutine) {
    mAwaitingCoroutine = awaitCoroutine;
    return mCount.fetch_sub(1, std::memory_order_acl_rel) > 1;
  }
  void Notify() noexcept {
    if (mCount.fetch_sub(1, std::memory_order_acl_rel) == 1) {
      mAwaitingCoroutine.resume();
    }
  }

 private:
  std::atomic<int> mCount;
  std::coroutine_handle<> mAwaitingCoroutine = nullptr;
};

template <typename T>
class WhenAllAwaitable;

template <>
class WhenAllAwaitable<std::tuple<>> {
 public:
  WhenAllAwaitable() noexcept {}
  WhenAllAwaitable(std::tuple<>) noexcept {}
  constexpr bool await_ready() noexcept { return true; }
  void await_suspend() noexcept {}
  std::tuple<> await_resume() noexcept { return {}; }
};

template <typename... Tasks>
class WhenAllAwaitable<std::tuple<Tasks...>> {
 public:
  WhenAllAwaitable(Tasks&&... tasks) : mTasks(std::forward_as_tuple(tasks...)), mCounter(sizeof...(Tasks)) {}
  explicit WhenAllAwaitable(std::tuple<Tasks...> input) : mTasks(std::move(input)), mCounter(sizeof...(Tasks)) {}
  explicit WhenAllAwaitable(WhenAllAwaitable&& other) : mTasks(std::move(other.mTasks)), mCounter(sizeof...(Tasks)) {}
  WhenAllAwaitable(const WhenAllAwaitable&) = delete;
  WhenAllAwaitable& operator=(const WhenAllAwaitable&) = delete;
  auto operator co_await() & noexcept {
    class Awaitable {
     public:
      bool await_ready() const noexcept { return mAwaitable.IsReady(); }
      bool await_suspend(std::coroutine_handle<> handle) noexcept { mAwaitable.TryAwait(handle); }
      std::tuple<Tasks...>& await_resume() { return mAwaitable.mTasks; }

     private:
      WhenAllAwaitable& mAwaitable;
    };
    return Awaitable{*this};
  }
  auto operator co_await() && noexcept {
    class Awaitbale {
     public:
      bool await_ready() const noexcept { return mAwaitable.IsReady(); }
      bool await_suspend(std::coroutine_handle<> handle) noexcept { mAwaitable.TryAwait(handle); }
      std::tuple<Tasks...>&& await_resume() { return std::move(mAwaitable.mTasks); }

     private:
      WhenAllAwaitable& mAwaitable;
    };
    return Awaitable{*this};
  }

 private:
  bool IsReady() { return mCounter.IsReady(); }
  bool TryAwait(std::coroutine_handle<> awaitingCoroutine) noexcept {
    StartTasks(std::make_integer_sequence<std::size_t, sizeof...(Tasks)>{});
    return mCounter.TryAwait(awaitingCoroutine);
  }
  template <std::size_t... Indices>
  void StartTasks(std::integer_sequence<std::size_t, Indices...>) noexcept {
    (void)std::initializer_list<int>{(std::get<Indices>(mTasks).Start(mCounter), 0)...};
  }
  WhenAllCounter mCounter;
  std::tuple<Tasks...> mTasks;
};

template <typename TasksContainer>
class WhenAllAwaitable<TasksContainer> {
 public:
  WhenAllAwaitable(TasksContainer&& tasksContainer) : mTasksContainer(std::forward<TasksContainer>(tasksContainer)), mCounter(tasksContainer.size()) {}
  WhenAllAwaitable(WhenAllAwaitable&& other) : mCounter(other.mTasksContainer.size()), mTasksContainer(std::move(other.mTasksContainer)) {}
  WhenAllAwaitable(const WhenAllAwaitable&) = delete;
  WhenAllAwaitable& operator=(const WhenAllAwaitable&) = delete;
  auto operator co_await() & noexcept {
    class Awaitable {
     public:
      bool await_ready() noexcept { return mAwaitable.IsReady(); }
      bool await_suspend(std::coroutine_handle<> handle) noexcept { return mAwaitable.TryAwait(handle); }
      TaskContainer& await_resume() noexcept { return mTasksContainer; }

     private:
      WhenAllAwaitable& mAwaitable;
    };
    return Awaitable { *this }
  }
  auto operator co_await() && noexcept {
    class Awaitable {
     public:
      bool await_ready() noexcept { return mAwaitable.IsReady(); }
      bool await_suspend(std::coroutine_handle<> handle) noexcept { return mAwaitable.TryAwait(handle); }
      TaskContainer&& await_resume() noexcept { return std::move(mTasksContainer); }

     private:
      WhenAllAwaitable& mAwaitable;
    };
    return Awaitable{*this};
  }

 private:
  bool IsReady() { return mCounter.IsReady(); }
  bool TryAwait(std::coroutine_handle<> awaitingCoroutine) noexcept {
    for (auto& task : mTasksContainer) {
      task.Start(mCounter);
    }
    return mCounter.TryAwait(awaitingCoroutine);
  }
  WhenAllCounter mCounter;
  TasksContainer mTasksContainer;
};

template <typename T>
class WhenAllTasks;

template <typename T>
class WhenAllPromise {
 public:
  using CoroutineHandle = std::coroutine_handle<WhenAllPromise<T>>;
  CoroutineHandle get_return_object() { return CoroutineHandle::from_promise(*this); }
  void return_void() { assert(false); }
  void Start(WhenAllCounter& counter) {
    mCounter = &counter;
    CoroutineHandle::from_promise(*this).resume();
  }
  auto initial_suspend() noexcept { return std::suspend_always{}; }
  void unhandled_exception() noexcept { mException = std::current_exception(); }
  auto yield_value(T&& value) {
    mResult = std::addressof(value);
    return final_suspend();
  }
  auto final_suspend() noexcept {
    class Awaitable {
     public:
      bool await_ready() const noexcept { return false; }
      void await_suspend(CoroutineHandle handle) const noexcept { handle.promise().mCounter->Notify(); }
      void await_resume() const noexcept {}
    };
    return Awaitable{};
  }
  T& get() & {
    if (mException) {
      std::rethrow_exception(mException);
    }
    return *mResult;
  }
  T&& get() && {
    if (mException) {
      std::rethrow_exception(mException);
    }
    return std::forward<T>(*mResult);
  }

 private:
  WhenAllCounter* mCounter;
  std::exception_ptr mException;
  T* mResult;
};

template <>
class WhenAllPromise<void> {
 public:
  using CoroutineHandle = std::coroutine_handle<WhenAllPromise<void>>;
  auto get_return_object() { CoroutineHandle::from_promise(*this); }
  auto initial_suspend() { return std::suspend_always{}; }
  auto final_suspend() {
    class Awaitable {
     public:
      bool await_ready() { return false; }
      void await_suspend(CoroutineHandle handle) { handle.promise().mCounter->Notify(); }
      void await_resume() {}
    };
    return Awaitable{};
  }
  void Start(WhenAllCounter& counter) {
    mCounter = &counter;
    CoroutineHandle::from_promise(*this).resume();
  }
  void return_void() {}
  void get() {
    if (mException) {
      std::rethrow_exception(mException);
    }
  }
  void unhandled_exception() { mException = std::current_exception(); }

 private:
  WhenAllCounter* mCounter;
  std::exception_ptr mException;
};

template <typename T>
class WhenAllTasks<T> {
 public:
  using promise_type = WhenAllPromise<T>;
  using CoroutineHandle = std::coroutine_handle<promise_type>;
  WhenAllTasks() noexcept {}
  WhenAllTasks(CoroutineHandle handle) noexcept : mCoroutineHandle(handle) {}
  WhenAllTasks(WhenAllTasks&& other) noexcept : mCoroutineHandle(std::exchange(other.mCoroutineHandle, CoroutineHandle{})) {}
  ~WhenAllTasks() {
    if (mCoroutineHandle) {
      mCoroutineHandle.destroy();
    }
  }
  decltype(auto) get() & { return mCoroutineHandle.promise().get(); }
  decltype(auto) get() && { return std::move(mCoroutineHandle.promise().get()); }
  decltype(auto) get_non_void_value() & {
    if constexpr (std::is_void<std::invoke_result_t<&WhenAllTasks::get>>::value) {
      get();
      return AnyData();
    } else {
      return get();
    }
  }
  decltype(auto) get_non_void_value() && {
    if constexpr (std::is_void<std::invoke_result_t<&WhenAllTasks::get>>::value) {
      get();
      return AnyData();
    } else {
      return std::move(get());
    }
  }

 private:
  template <typename Tasks>
  friend class WhenAllAwaitable;
  void Start(WhenAllCounter& counter) { mCoroutineHandle.promise().Start(counter); }
  WhenAllTasks(const WhenAllTasks&) = delete;
  WhenAllTasks& operator=(const WhenAllTasks&) = delete;
  std::coroutine_handle<promise_type> mCoroutineHandle;
};

template <typename... Awaitables, typename F = WhenAllAwaitable<Awaitables...>, typename Result = typename awaitable_traits<F>::await_result_t, std::enable_if_t<!std::is_void<Result>::value, int> = 0>
WhenAllTasks<Result> make_when_all_tasks(Awaitables... awaitables) {
  WhenAllAwaitable allAwaitable(std::forward<Awaitables>(awaitables)...);
  co_yield co_await std::forward<WhenAllAwaitable>(allAwaitable)
}
template <typename... Awaitables, typename F = WhenAllAwaitable<Awaitables...>, typename Result = typename awaitable_traits<F>::await_result_t, std::enable_if_t<std::is_void<Result>::value, int> = 0>
WhenAllTasks<void> make_when_all_tasks(Awaitables... awaitables) {
  WhenAllAwaitable allAwaitable(std::forward<Awaitables>(awaitables)...);
  co_await std::forward<WhenAllAwaitable>(allAwaitable);
}
template <typename AwaitableT, typename Result = typename awaitable_traits<AwaitableT>::await_result_t, std::enable_if_t<!std::is_void<Result>::value, int> = 0>
WhenAllTasks<Result> make_when_all_tasks(AwaitableT&& awaitable) {
  co_yield co_await std::forward<AwaitableT>(awaitable);
}
template <typename AwaitableT, typename Result = typename awaitable_traits<AwaitableT>::await_result_t, std::enable_if_t<std::is_void<Result>::value, int> = 0>
WhenAllTasks<void> make_when_all_tasks(AwaitableT&& awaitable) {
  co_await std::forward<AwaitableT>(awaitable);
}

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_INTERNAL_WHEN_ALL_ */
