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
  (void)num;
  float res = 0.0;
  for (int i = 0; i < 1000; ++i) {
    res += (float)i / 10.2;
  }
  return res;
}

void Fn1(async::CommonAsyncKernelFrame *frame) { (void)frame; }
void Fn2(async::CommonAsyncKernelFrame *frame) {
  frame->EmplaceResult<int>(LargeComputeFn(frame->GetArgAt<int>(0)));
}

using KernelFnPtr = void (*)(async::CommonAsyncKernelFrame *frame);

int main() {
  std::cout << "hardware concurrency number! "
            << std::thread::hardware_concurrency() << "\n";
  auto runContext =
      CreateCustomHostContext(std::thread::hardware_concurrency(), 1);
  RCReference<AsyncGraph> graph = CreateAsyncGraph(runContext.get());
  REGISTER_KERNEL_FN("start", Fn1);
  REGISTER_KERNEL_FN("run", Fn2);
  for (int i = 0; i < 100; ++i) {
    graph->emplace({"output"}, {"result" + std::to_string(i)},
                   GET_KERNEL_FN("run").value(), "run");
  }
  graph->emplace({}, {"output"}, GET_KERNEL_FN("start").value(), "start");
  graph->BuildGraph();

  int numIters = 10;
  int iterRangeValue = 100000;
  std::vector<std::vector<RCReference<AsyncValue>>> results;
  results.resize(numIters + 1);
  results[0].push_back(
      runContext->MakeAvailableAsyncValueRef<int>(iterRangeValue));
  auto start = high_resolution_clock::now();
  for (int i = 0; i < numIters; ++i) {
    RunAsyncGraph(graph.get(), results[i], results[i + 1], true);
  }
  runContext->Await(results[numIters]);
  for (const auto &elem : results[numIters]) {
    std::cout << elem->get<int>() << std::endl;
  }
  auto end = high_resolution_clock::now();
  std::cout << duration_cast<nanoseconds>(end - start).count() << std::endl;

  graph->Dump("./graph.txt");
  graph->Reset();
  graph->Load("./graph.txt");
  graph->BuildGraph();
  graph->Dump("./graph2.txt");
  std::vector<RCReference<AsyncValue>> input;
  input.push_back(runContext->MakeAvailableAsyncValueRef<int>(0));
  std::vector<RCReference<AsyncValue>> output;
  start = high_resolution_clock::now();
  RunAsyncGraph(graph.get(), input, output, true);
  end = high_resolution_clock::now();
  std::cout << "async time : "
            << duration_cast<nanoseconds>(end - start).count() << std::endl;
  fs::remove("./graph.txt");
  RCReference<AsyncGraph> subGraph =
      graph->SubGraph(std::vector<std::string>{"result1", "result2"});
  subGraph->BuildGraph();
  subGraph->Dump("./sub_graph.txt");
  runContext->Await(output);
  std::cout << output[0]->get<int>() << std::endl;
  return 0;
}