#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../context/host_context.h"
#include "../context/native_function.h"
#include "common/threading/thread_pool.h"

using namespace ficus;
using namespace async;

void LargeCompute(AsyncValue* const* inputs, int numArguments,
                  RCReference<AsyncValue>* result, int numResult,
                  HostContext* ctx) {
  auto& vec = inputs[0]->get<std::vector<int>>();
  result[0] = std::move(ctx->EnqueueWork([&vec]() {
    for (int i = 0; i < vec.size(); ++i) {
      vec[i] += 1;
    }
    return std::move(vec);
  }));
}

int SyncLargeCompute(std::vector<int>& input, std::vector<int>& output) {
  for (int i = 0; i < input.size(); ++i) {
    input[i] += 1;
  }
  output = std::move(input);
  return 0;
}

static auto Now() { return std::chrono::steady_clock::now(); }

int main() {
  auto context = CreateSimpleHostContext();
  RCReference<const SimpleFunction> func =
      TakeRef(new SimpleFunction("LargeCompute", LargeCompute));
  llvm::SmallVector<RCReference<AsyncValue>, 4> results;
  int numEle = 2000;
  results.resize(numEle);
  auto input1 =
      context->MakeAvailableAsyncValueRef<std::vector<int>>(100000, 0);

  std::vector<std::vector<int>> input;
  input.resize(numEle + 1);
  input[0] = std::vector<int>(100000, 0);
  ThreadPool pool(std::thread::hardware_concurrency(), 1000);
  auto start1 = Now();
  for (int i = 0; i < numEle; ++i) {
    int rtn;
    ThreadPool::TaskPtr task = boost::make_shared<ThreadPool::Task>(
        [&input, i]() { return SyncLargeCompute(input[i], input[i + 1]); });
    pool.Submit(task);
    task->Wait(rtn);
  }
  auto end1 = Now();
  std::cout << "Old Ficus Run Time:  "
            << std::chrono::duration_cast<std::chrono::microseconds>(end1 -
                                                                     start1)
                   .count()
            << std::endl;

  auto start2 = Now();
  func->Execute(llvm::ArrayRef<AsyncValue*>{input1.GetAsyncValue()},
                {results[0]}, context.get());
  for (int i = 0; i < numEle - 1; ++i) {
    llvm::SmallVector<AsyncValue*, 4> mappedVec;
    mappedVec.push_back(results[i].get());
    func->Execute(mappedVec, {results[i + 1]}, context.get());
  }
  context->Quiesce();
  auto end2 = Now();
  std::cout << "New Async Run Time:  "
            << std::chrono::duration_cast<std::chrono::microseconds>(end2 -
                                                                     start2)
                   .count()
            << std::endl;
  return 0;
}
