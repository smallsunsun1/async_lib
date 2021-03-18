#ifndef ASYNC_COROUTINE_TASK_
#define ASYNC_COROUTINE_TASK_

#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace sss {
namespace async {
template <typename T>
class Task;
namespace internal {
class BasePromise {
  friend class Awaitable;
  struct Awaitable {
    bool await_ready() noexcept { return false; }
    template <typename PromiseT>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseT> handle) noexcept {
      return handle.promise().mContinuation;
    }
    void await_resume() noexcept {}
  };

 public:
  BasePromise() noexcept {}
  auto initial_suspend() noexcept { return std::suspend_always{}; }
  auto final_suspend() noexcept { return Awaitable{}; }
  void SetContinuation(std::coroutine_handle<> continuation) noexcept { mContinuation = continuation; }

 private:
  std::coroutine_handle<> mContinuation;
};
template <typename T>
class TaskPromise : public BasePromise {
 public:
  TaskPromise() noexcept {}
  ~TaskPromise() {
    switch (mResultType) {
      case ResultType::kAvaliable:
        mValue.~T();
        break;
      case ResultType::kException:
        mException.~exception_ptr();
        break;
      default:
        break;
    }
  }
  Task<T> get_return_object() noexcept;
  void unhandled_exception() noexcept {
    new (static_cast<void*>(std::addressof(mException))) std::exception_ptr(std::current_exception());
    mResultType = ResultType::kException;
  }
  template <typename U, std::enable_if_t<std::is_convertible<U, T>::value, int> N = 0>
  void return_value(U&& value) noexcept {
    new (static_cast<void*>(std::addressof(mValue))) T(std::forward<U>(value));
    mResultType = ResultType::kAvaliable;
  }
  T& get() & {
    if (mResultType == ResultType::kException) {
      std::rethrow_exception(mException);
    }
    assert(mResultType == ResultType::kAvaliable);
    return mValue;
  }
  using return_value_type = std::conditional_t<std::is_arithmetic<T>::value, T, T&&>;
  return_value_type get() && {
    if (mResultType == ResultType::kException) {
      std::rethrow_exception(mException);
    }
    assert(mResultType == ResultType::kAvaliable);
    return std::move(mValue);
  }

 private:
  enum class ResultType { kEmpty = 0, kAvaliable = 1, kException = 2 };
  ResultType mResultType = ResultType::kEmpty;
  union {
    T mValue;
    std::exception_ptr mException;
  };
};
template <>
class TaskPromise<void> : public BasePromise {
 public:
  TaskPromise() noexcept = default;
  Task<void> get_return_object() noexcept;
  void return_void() noexcept {}
  void unhandled_exception() { mException = std::current_exception(); }
  void get() {
    if (mException) {
      std::rethrow_exception(mException);
    }
  }

 private:
  std::exception_ptr mException;
};
template <typename T>
class TaskPromise<T&> : public BasePromise {
 public:
  TaskPromise() = default;
  Task<T&> get_return_object() noexcept;
  void return_value(T& value) noexcept { mValue = std::addressof(&value); }
  void unhandled_exception() { mException = std::current_exception(); }
  T& get() {
    if (mException) {
      std::rethrow_exception(mException);
    }
    return *mValue;
  }

 private:
  T* mValue;
  std::exception_ptr mException;
};
}  // namespace internal
template <typename T = void>
class Task {
 public:
  using promise_type = internal::TaskPromise<T>;
  using value_type = T;
  Task() noexcept : mCoroutine(nullptr) {}
  explicit Task(std::coroutine_handle<promise_type> coroutine) : mCoroutine(coroutine) {}
  Task(Task&& t) noexcept : mCoroutine(t.mCoroutine) { t.mCoroutine = nullptr; }
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  ~Task() {
    if (mCoroutine) {
      mCoroutine.destroy();
    }
  }
  Task& operator=(Task&& other) noexcept {
    if (std::addressof(other) != this) {
      if (mCoroutine) {
        mCoroutine.destroy();
      }
      mCoroutine = other.mCoroutine;
      other.mCoroutine = nullptr;
    }
    return *this;
  }
  bool is_ready() const noexcept { return !mCoroutine || mCoroutine.done(); }
  auto operator co_await() const& noexcept {
    struct Awaitable : AwaitableBase {
      using AwaitableBase::AwaitableBase;
      decltype(auto) await_resume() {
        if (!this->mCoroutine) {
          throw std::runtime_error("empty coroutine is not accepted!");
        }
        return this->mCoroutine.promise().get();
      }
    };
    return Awaitable{mCoroutine};
  }
  auto operator co_await() const&& noexcept {
    struct Awaitable : AwaitableBase {
      using AwaitableBase::AwaitableBase;
      decltype(auto) await_resume() {
        if (!this->mCoroutine) {
          throw std::runtime_error("empty coroutine is not accepted!");
        }
        return this->mCoroutine.promise().get();
      }
    };
    return Awaitable{mCoroutine};
  }
  auto when_ready() const noexcept {
    struct Awaitable : AwaitableBase {
      using AwaitableBase::AwaitableBase;
      void await_resume() const noexcept {}
    };
    return Awaitable{mCoroutine};
  }

 private:
  struct AwaitableBase {
    std::coroutine_handle<promise_type> mCoroutine;
    AwaitableBase(std::coroutine_handle<promise_type> coroutine) noexcept : mCoroutine(coroutine) {}
    bool await_ready() const noexcept { return !mCoroutine || mCoroutine.done(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
      mCoroutine.promise().SetContinuation(awaitingCoroutine);
      return mCoroutine;
    }
  };

 private:
  std::coroutine_handle<promise_type> mCoroutine;
};
namespace internal {
template <typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept {
  return Task<T>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}
inline Task<void> TaskPromise<void>::get_return_object() noexcept { return Task<void>{std::coroutine_handle<TaskPromise>::from_promise(*this)}; }
template <typename T>
Task<T&> TaskPromise<T&>::get_return_object() noexcept {
  return Task<T&>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}
}  // namespace internal

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_TASK_ */
