#include <iostream>

#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/context/kernel_frame.h"
#include "async/runtime/graph.h"

using namespace sss;
using namespace async;

int main() {
  auto runContext = CreateCustomHostContext(20, 1);
  AsyncGraph *memory = runContext->Allocate<AsyncGraph>();
  AsyncGraph *graphOri = new (memory) AsyncGraph(runContext.get());
  RCReference<AsyncGraph> graph = TakeRef(graphOri);
  graph->emplace({"output"}, {"result"},
                 [](async::CommonAsyncKernelFrame *kernel) {
                   std::cout << "run code\n";
                   kernel->EmplaceResult<async::Chain>();
                 });
  graph->emplace({}, {"output"},
                 [](async::CommonAsyncKernelFrame *kernel) { (void)kernel; });
  graph->BuildGraph();
  for (int i = 0; i < 100000; ++i) {
    std::vector<RCReference<AsyncValue>> oriArguments;
    oriArguments.push_back(
        runContext->MakeAvailableAsyncValueRef<async::Chain>());
    std::vector<RCReference<AsyncValue>> results;
    std::vector<RCReference<AsyncValue>> results2;
    RunAsyncGraph(graph.get(), oriArguments, results, false);
    RunAsyncGraph(graph.get(), results, results2, false);
    runContext->Await(results2);
  }
}
