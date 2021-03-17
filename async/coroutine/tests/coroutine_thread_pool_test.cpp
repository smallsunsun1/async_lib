#include <iostream>
#include "async/coroutine/coroutine_thread_pool.h"
#include "async/coroutine/task.h"

using namespace sss;
using namespace async;

Task<void> DoSimpleWorkOnThreadPool(CoroutineThreadPool& pool) {
    co_await pool.Schedule();
    std::cout << "Hello World!\n";
}

int main() {
    CoroutineThreadPool pool(10);
    Task<void> res = DoSimpleWorkOnThreadPool(pool);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}