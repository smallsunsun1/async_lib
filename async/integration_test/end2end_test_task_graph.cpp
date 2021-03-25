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