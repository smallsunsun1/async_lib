#include "async/coroutine/coroutine_thread_pool.h"

#include <iostream>

#include "async/coroutine/coroutine_waitable_task.h"
#include "async/coroutine/task.h"

using namespace sss;
using namespace async;

Task<void> DoSimpleWorkOnThreadPool(CoroutineThreadPool& pool) {
  co_await pool.Schedule();
  std::cout << "Hello World!\n";
}

Task<int> DoSimpleReturnValueOnThreadPool(CoroutineThreadPool& pool) {
  co_await pool.Schedule();
  co_return 100;
}

int main() {
  CoroutineThreadPool pool(10);
  Task<void> res = DoSimpleWorkOnThreadPool(pool);
  Task<int> res2 = DoSimpleReturnValueOnThreadPool(pool);
  SyncWait(res);
  int result = SyncWait(res2);
  std::cout << result << std::endl;
}