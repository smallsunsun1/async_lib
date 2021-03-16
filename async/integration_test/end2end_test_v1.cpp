#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/context/kernel_frame.h"
#include "async/runtime/graph.h"
#include "async/support/ref_count.h"

using namespace ficus;
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
  // for (int i = 0; i < numIters * 100; ++i) {
  //     LargeComputeFn(iterRangeValue);
  // }
  auto end = high_resolution_clock::now();
  std::cout << duration_cast<nanoseconds>(end - start).count();
}