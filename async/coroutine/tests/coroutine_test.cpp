#include <cassert>
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "task.h"

auto switch_to_new_thread(std::jthread& out) {
  struct awaitable {
    std::jthread* p_out;
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
      std::jthread& out = *p_out;
      if (out.joinable()) assert(false && "out thread can't be joinable!");
      out = std::jthread([h] { h.resume(); });
      // Potential undefined behavior: accessing potentially destroyed *this
      // std::cout << "New thread ID: " << p_out->get_id() << '\n';
      std::cout << "New thread ID: " << out.get_id() << '\n';  // this is OK
    }
    void await_resume() {}
  };
  return awaitable{&out};
}

struct task {
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

task resuming_on_new_thread(std::jthread& out) {
  std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
  co_await switch_to_new_thread(out);
  // awaiter destroyed here
  std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << '\n';
}

struct TestAwaitable {
  bool await_ready() { return false; }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) { return h; }
  int await_resume() { return 100; }
};

using namespace sss;
using namespace async;

Task<int> TaskTest() {
  std::cout << "hello world!\n";
  int n = co_await TestAwaitable{};
  std::cout << "hello world!\n";
  co_return n;
}

Task<> Example() {
  Task<int> countTask = TaskTest();
  int result = co_await countTask;
  std::cout << "final count = " << result << std::endl;
}

int main() {
  // std::jthread out;
  // resuming_on_new_thread(out);
  Example();
}