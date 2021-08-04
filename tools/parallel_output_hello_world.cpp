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

  // for (int ii = 0; ii < 2; ++ii) {
  //     AsyncGraph* memory = runContext->Allocate<AsyncGraph>();
  //     AsyncGraph* graphOri = new (memory) AsyncGraph(runContext.get());
  //     RCReference<AsyncGraph> graph = TakeRef(graphOri);
  //     absl::InlinedVector<RCReference<AsyncValue>, 4> oriArguments;
  //     oriArguments.push_back(runContext->MakeAvailableAsyncValueRef<async::Chain>());
  //     graph->emplace_back({}, {"output"}, [](async::CommonAsyncKernelFrame*
  //     kernel){
  //     });
  //     graph->emplace_back({"output"}, {"result"},
  //     [](async::CommonAsyncKernelFrame* kernel){
  //         std::cout << "run code\n";
  //         kernel->EmplaceResult<async::Chain>();
  //     });
  //     graph->BuildGraph();
  //     GraphExecutor* execPtr = runContext->Allocate<GraphExecutor>();
  //     GraphExecutor* exec = new (execPtr) GraphExecutor(graph.get());
  //     absl::InlinedVector<AsyncValue*, 8> arguments;
  //     arguments.push_back(oriArguments[0].get());
  //     absl::InlinedVector<RCReference<AsyncValue>, 4> results;
  //     results.resize(graph->GetNumOutputs());
  //     GraphExecutor::Execute(exec, absl::MakeConstSpan(arguments.data(),
  //     arguments.size()),
  //                             absl::MakeSpan(results.data(),
  //                             results.size()));
  //     runContext->Await(results);
  //     std::cout << results.size() << std::endl;
  //     std::cout << results[0]->NumCount() << "\n";
  //     graph->Dump("./struct.json");

  //     AsyncGraph* memory2 = runContext->Allocate<AsyncGraph>();
  //     AsyncGraph* graphOri2 = new (memory2) AsyncGraph(runContext.get());
  //     RCReference<AsyncGraph> graph2 = TakeRef(graphOri2);
  //     graph2->Load("./struct.json");
  //     graph2->Dump("./struct2.json");
  // }
}
