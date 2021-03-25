一: 说明
当前图执行框架已经基本上完成，主要分为TaskGraph模式和AsyncGraph模式  
具体代码参考async/runtime/graph.h和async/runtime/task_graph.h文件

二：TaskGraph模式
TaskGraph：描述整个图的数据结构，记录了不同Node之间的依赖关系
TaskNode：Task所在的载体，TaskNode通过AddSuccessor和AddDependency函数对不同的Node之间进行依赖编排
TaskNode必须通过TaskGraph的emplace_back函数产生，只有这种情况下，TaskGraph才会对TaskNode进行了记录，并负责对应TaskNode资源的释放
TaskGraphExecutor：实际执行TaskGraph的执行器

example：
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/runtime/task_graph.h"
#include "async/support/ref_count.h"

using namespace sss;
using namespace async;
using namespace std::chrono;

int LargeComputeFn(int num) {
  float res;
  for (int i = 0; i < num; ++i) {
    res += (float)i / 10.2;
  }
  return res;
}

int main() {
  std::cout << "hardware concurrency number! " << std::thread::hardware_concurrency() << "\n";
  auto runContext = CreateCustomHostContext(std::thread::hardware_concurrency(), 1);
  TaskGraph* memory = runContext->Allocate<TaskGraph>();
  TaskGraph* graphOri = new (memory) TaskGraph(runContext.get());
  RCReference<TaskGraph> graph = TakeRef(graphOri);
  std::vector<TaskNode*> nodes;
  for (int i = 0; i < 100; ++i) {
      TaskNode* node = graph->emplace_back([i](){
          std::cout << LargeComputeFn(i) << std::endl;
      });
      nodes.push_back(node);
  }
  for (int i = 0; i < nodes.size() - 1; ++i) {
      nodes[i]->AddSuccessor(nodes[i+1]);
  }
  graph->BuildGraph();

  auto start = high_resolution_clock::now();
  RunTaskGraph(graph.get(), true);
  auto end = high_resolution_clock::now();
  std::cout << duration_cast<nanoseconds>(end - start).count();
}

三：AsyncGraph模式
AsyncGraph是一种更高效的异步执行图架构，图中的每个节点对应相应的Kernel，不同Kernel之间由不同
的边进行连接，称为AsyncValue。GraphExecutor会负责所有AsyncValue的内存释放，用户只需定义完AsyncGraph
的图结构，和对应的输入和输出参数即可。
AsyncGraph：描述整个图的数据结构，记录了不同Node和edge之间的关系
AsyncNode：描述了对应Node的Task函数，以及Node的输入边名称和输出边名称
GraphExecutor：实际执行AsyncGraph运行的执行器

example：
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/context/kernel_frame.h"
#include "async/runtime/graph.h"
#include "async/support/ref_count.h"

using namespace sss;
using namespace async;
using namespace std::chrono;

int LargeComputeFn(int num) {
  float res;
  for (int i = 0; i < num; ++i) {
    res += (float)i / 10.2;
  }
  return res;
}

int main() {
  std::cout << "hardware concurrency number! " << std::thread::hardware_concurrency() << "\n";
  auto runContext = CreateCustomHostContext(std::thread::hardware_concurrency(), 1);
  AsyncGraph* memory = runContext->Allocate<AsyncGraph>();
  AsyncGraph* graphOri = new (memory) AsyncGraph(runContext.get());
  RCReference<AsyncGraph> graph = TakeRef(graphOri);
  graph->emplace_back({}, {"output"}, [](async::CommonAsyncKernelFrame* frame) {});
  for (int i = 0; i < 100; ++i) {
    graph->emplace_back({"output"}, {"result" + std::to_string(i)}, [](async::CommonAsyncKernelFrame* frame) {
      LargeComputeFn(frame->GetArgAt<int>(0));
      frame->EmplaceResult<int>(100);
    });
  }
  graph->BuildGraph();

  int numIters = 10;
  int iterRangeValue = 100000;
  std::vector<std::vector<RCReference<AsyncValue>>> results;
  results.resize(numIters + 1);
  results[0].push_back(runContext->MakeAvailableAsyncValueRef<int>(iterRangeValue));
  auto start = high_resolution_clock::now();
  for (int i = 0; i < numIters; ++i) {
    RunAsyncGraph(graph.get(), results[i], results[i + 1], true);
  }
  runContext->Await(results[numIters]);
  auto end = high_resolution_clock::now();
  std::cout << duration_cast<nanoseconds>(end - start).count();
}

4. 实现原理