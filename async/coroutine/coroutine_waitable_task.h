#ifndef ASYNC_COROUTINE_COROUTINE_WAITABLE_TASK_
#define ASYNC_COROUTINE_COROUTINE_WAITABLE_TASK_

#include <atomic>
#include <coroutine>
#include <exception>
#include <stdexcept>

#include "coroutine_notifier.h"
#include "coroutine_traits.h"

namespace sss {
namespace async {

template <typename T>
class WaitableTask;

template <typename T>
class WaitableTaskPromise {
  using CoroutineHandle = std::coroutine_handle<WaitableTaskPromise<T>>;

 public:
  WaitableTaskPromise() noexcept {}
  void Start(CoroutineNotifier& event) {
    mEvent = &event;
    CoroutineHandle::from_promise(*this).resume();
  }
  auto get_return_object() noexcept { return CoroutineHandle::from_promise(*this); }
  auto initial_suspend() noexcept { return std::suspend_always{}; }
  auto final_suspend() noexcept {
    class AwaitableNotifier {
     public:
      bool await_ready() noexcept { return false; }
      void await_suspend(CoroutineHandle handle) const noexcept { handle.promise().mEvent->Set(); }
      void await_resume() noexcept {}
    };
    return AwaitableNotifier{};
  }
  auto yield_value(T&& result) noexcept {
    mData = std::addressof(result);
    return final_suspend();
  }
  void return_void() noexcept { assert(false); }
  void unhandled_exception() { mException = std::current_exception(); }
  T&& get() {
    if (mException) {
      std::rethrow_exception(mException);
    }
    return static_cast<T&&>(*mData);
  }

 private:
  CoroutineNotifier* mEvent;
  std::remove_reference_t<T>* mData;
  std::exception_ptr mException;
};
template <>
class WaitableTaskPromise<void> {
  using CoroutineHandle = std::coroutine_handle<WaitableTaskPromise<void>>;

 public:
  WaitableTaskPromise() noexcept {}
  void Start(CoroutineNotifier& event) {
    mEvent = &event;
    CoroutineHandle::from_promise(*this).resume();
  }
  auto get_return_object() noexcept { return CoroutineHandle::from_promise(*this); }
  auto initial_suspend() noexcept { return std::suspend_always{}; }
  auto final_suspend() noexcept {
    class AwaitableNotifier {
     public:
      bool await_ready() noexcept { return false; }
      void await_suspend(CoroutineHandle handle) const noexcept { handle.promise().mEvent->Set(); }
      void await_resume() noexcept {}
    };
    return AwaitableNotifier{};
  }
  void return_void() {}
  void unhandled_exception() { mException = std::current_exception(); }
  void get() {
    if (mException) {
      std::rethrow_exception(mException);
    }
  }

 private:
  CoroutineNotifier* mEvent;
  std::exception_ptr mException;
};

template <typename T>
class WaitableTask {
 public:
  using promise_type = WaitableTaskPromise<T>;
  using CoroutineHandle = std::coroutine_handle<promise_type>;
  WaitableTask(CoroutineHandle coroutine) : mCoroutine(coroutine) {}
  WaitableTask(WaitableTask&& task) noexcept : mCoroutine(std::exchange(task.mCoroutine, CoroutineHandle{})) {}
  ~WaitableTask() {
    if (mCoroutine) {
      mCoroutine.destroy();
    }
  }
  WaitableTask(const WaitableTask&) = delete;
  WaitableTask& operator=(const WaitableTask&) = delete;
  void Start(CoroutineNotifier& event) noexcept { mCoroutine.promise().Start(event); }
  decltype(auto) get() { return mCoroutine.promise().get(); }

 private:
  CoroutineHandle mCoroutine;
};
template <typename Awaitable, typename Result = typename awaitable_traits<Awaitable&&>::await_result_t, std::enable_if_t<!std::is_void<Result>::value, int> = 0>
WaitableTask<Result> make_waitable_task(Awaitable&& awaitable) {
  co_yield co_await std::forward<Awaitable>(awaitable);
}
template <typename Awaitable, typename Result = typename awaitable_traits<Awaitable&&>::await_result_t, std::enable_if_t<std::is_void<Result>::value, int> = 0>
WaitableTask<void> make_waitable_task(Awaitable&& awaitable) {
  co_await std::forward<Awaitable>(awaitable);
}

template <typename Awaitable>
auto SyncWait(Awaitable&& awaitable) -> typename awaitable_traits<Awaitable&&>::await_result_t {
  auto task = make_waitable_task(std::forward<Awaitable>(awaitable));
  CoroutineNotifier event;
  task.Start(event);
  event.Wait();
  return task.get();
}

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_COROUTINE_WAITABLE_TASK_ */
