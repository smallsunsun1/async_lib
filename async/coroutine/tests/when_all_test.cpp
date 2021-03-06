#include "async/coroutine/when_all.h"

#include <iostream>

#include "async/coroutine/coroutine_thread_pool.h"
#include "async/coroutine/coroutine_waitable_task.h"
#include "async/coroutine/fmap.h"
#include "async/coroutine/task.h"
#include "async/coroutine/when_all_ready.h"

using namespace sss;
using namespace async;

Task<void> DoSimpleWorkOnThreadPool(CoroutineThreadPool &pool) {
  co_await pool.Schedule();
  std::cout << std::this_thread::get_id() << std::endl;
  std::cout << "Hello World!\n";
}

Task<int> DoSimpleReturnValueOnThreadPool(CoroutineThreadPool &pool) {
  co_await pool.Schedule();
  std::cout << std::this_thread::get_id() << std::endl;
  co_return 100;
}

int main() {
  CoroutineThreadPool pool(10);
  Task<void> res = DoSimpleWorkOnThreadPool(pool);
  Task<int> res2 = DoSimpleReturnValueOnThreadPool(pool);
  auto res3 = WhenAll(std::move(res), std::move(res2));
  auto finalRes = SyncWait(res3);
  std::cout << std::get<1>(finalRes) << std::endl;
  return 0;
}