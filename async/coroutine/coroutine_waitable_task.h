#ifndef ASYNC_COROUTINE_COROUTINE_WAITABLE_TASK_
#define ASYNC_COROUTINE_COROUTINE_WAITABLE_TASK_

#include <stdexcept>
#include <exception>
#include <coroutine>
#include <atomic>

#include "coroutine_notifier.h"

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
        mEvent = event;
        CoroutineHandle::from_promise(*this).resume();
    }
    auto get_return_object() noexcept {
        return CoroutineHandle::from_promise(*this).resume();
    }
    auto initial_suspend() noexcept {
        return std::suspend_always{};
    }
    auto final_suspend() noexcept {
        class AwaitableNotifier {
            bool await_ready() noexcept {return false;}
            void await_suspend(CoroutineHandle handle) const noexcept {
                handle.promise().mEvent->Set();
            }
            void await_resume() noexcept {}
        };
        return AwaitableNotifier{};
    }
    auto yield_value(T&& result) noexcept {
        mData = std::addressof(result);
        return final_suspend{};
    }
    void unhandled_exception() {
        mException = std::current_exception();
    }
    T&& get() {
        if (mException) {
            std::rethrow_exception(mException);
        }
        return static_cast<T&&>(*mData);
    }
private:
    CoroutineNotifier* mEvent;
    T* mData;
    std::exception_ptr mException;
};
template <>
class WaitableTaskPromise<void> {
    using CoroutineHandle = std::coroutine_handle<WaitableTaskPromise<void>>;
public:
    WaitableTaskPromise() noexcept {}
    void Start(CoroutineNotifier& event) {
        mEvent = event;
        CoroutineHandle::from_promise(*this).resume();
    }
    auto get_return_object() noexcept {
        return CoroutineHandle::from_promise(*this).resume();
    }
    auto initial_suspend() noexcept {
        return std::suspend_always{};
    }
    auto final_suspend() noexcept {
        class AwaitableNotifier {
            bool await_ready() noexcept {return false;}
            void await_suspend(CoroutineHandle handle) const noexcept {
                handle.promise().mEvent->Set();
            }
            void await_resume() noexcept {}
        };
        return AwaitableNotifier{};
    }
    auto yield_value(T&& result) noexcept {
        mData = std::addressof(result);
        return final_suspend{};
    }
    void unhandled_exception() {
        mException = std::current_exception();
    }
    T&& get() {
        if (mException) {
            std::rethrow_exception(mException);
        }
        return static_cast<T&&>(*mData);
    }
private:
    CoroutineNotifier* mEvent;
    T* mData;
    std::exception_ptr mException;
}

}
}

#endif /* ASYNC_COROUTINE_COROUTINE_WAITABLE_TASK_ */
