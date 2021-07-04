#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/context/kernel_frame.h"
#include "async/runtime/graph.h"
#include "async/runtime/register.h"
#include "async/support/ref_count.h"

using namespace sss;
using namespace async;
using namespace std::chrono;
namespace fs = std::filesystem;

int LargeComputeFn(int num) {
  float res = 0.0;
  for (int i = 0; i < num; ++i) {
    res += (float)i / 10.2;
  }
  return res;
}

void Fn1(async::CommonAsyncKernelFrame* frame) { (void)frame; }
void Fn2(async::CommonAsyncKernelFrame* frame) {
  LargeComputeFn(frame->GetArgAt<int>(0));
  frame->EmplaceResult<int>(100);
}

using KernelFnPtr = void (*)(async::CommonAsyncKernelFrame* frame);

int main() {
  std::cout << "hardware concurrency number! " << std::thread::hardware_concurrency() << "\n";
  auto runContext = CreateCustomHostContext(std::thread::hardware_concurrency(), 1);
  RCReference<AsyncGraph> graph = CreateAsyncGraph(runContext.get());
  REGISTER_KERNEL_FN("start", Fn1);
  REGISTER_KERNEL_FN("run", Fn2);
  for (int i = 0; i < 100; ++i) {
    graph->emplace_back({"output"}, {"result" + std::to_string(i)}, GET_KERNEL_FN("run").value(), "run", true);
  }
  graph->emplace_back({}, {"output"}, GET_KERNEL_FN("start").value(), "start", true);
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
  std::cout << duration_cast<nanoseconds>(end - start).count() << std::endl;

  graph->Dump("./graph.txt");
  graph->Reset();
  graph->Load("./graph.txt");
  graph->BuildGraph();
  std::vector<RCReference<AsyncValue>> input;
  input.push_back(runContext->MakeAvailableAsyncValueRef<int>(0));
  std::vector<RCReference<AsyncValue>> output;
  RunAsyncGraph(graph.get(), input, output, true);
  std::cout << output[0]->get<int>() << std::endl;
  fs::remove("./graph.txt");
  return 0;
}