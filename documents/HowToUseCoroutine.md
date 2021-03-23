一: 说明
当前Coroutine的代码还处于未完成的状态，只支持最简单的ThreadPool提交任务，并在特定的位置同步等待相应Task完成的操作  
如果需要使用coroutine，使用gcc10以上版本的编译器，同时cmake编译时开启-DUSE_CXX_20=ON来开启c++20的编译  

一个简单的Sample：

//////////////////////////////////////////////////

#include <iostream>  
#include "async/coroutine/coroutine_thread_pool.h"  
#include "async/coroutine/coroutine_waitable_task.h"  
#include "async/coroutine/task.h"  

using namespace sss;  
using namespace async;  

// 定义一个返回值为void的coroutine函数  
Task<void> DoSimpleWorkOnThreadPool(CoroutineThreadPool& pool) {  
  co_await pool.Schedule();  
  std::cout << std::this_thread::get_id() << std::endl;  
  std::cout << "Hello World!\n";  
}  

// 定义一个返回值为int的coroutine函数  
Task<int> DoSimpleReturnValueOnThreadPool(CoroutineThreadPool& pool) {  
  co_await pool.Schedule();  
  std::cout << std::this_thread::get_id() << std::endl;  
  co_return 100;  
}  

int main() {  
  CoroutineThreadPool pool(10);  // 初始化协程线程池  
  Task<void> res = DoSimpleWorkOnThreadPool(pool);   // 返回Task结果  
  Task<int> res2 = DoSimpleReturnValueOnThreadPool(pool);  // 返回Task结果    
  SyncWait(res);  //  同步等待协程DoSimpleWorkOnThreadPool执行完成  
  int result = SyncWait(res2);  // 同步等待DoSimpleReturnValueOnThreadPool执行完成  
  std::cout << result << std::endl;  // 打印输出结果，输出结果应该为100  
}

//////////////////////////////////////////////////  