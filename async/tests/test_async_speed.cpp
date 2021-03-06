#include <algorithm>    // for max
#include <chrono>       // for microseconds, duration_cast
#include <iostream>     // for operator<<, endl, basic_o...
#include <memory>       // for unique_ptr
#include <type_traits>  // for remove_reference<>::type
#include <utility>      // for move
#include <vector>       // for vector

#include "absl/memory/memory.h"             // for allocator_traits<>::value...
#include "absl/types/span.h"                // for MakeSpan, MakeConstSpan
#include "async/context/async_value.h"      // for AsyncValue
#include "async/context/async_value_ref.h"  // for AsyncValueRef
#include "async/context/host_context.h"     // for CreateSimpleHostContext
#include "async/context/native_function.h"  // for SimpleFunction
#include "async/support/ref_count.h"        // for RCReference, TakeRef, async

using namespace sss;
using namespace async;

void LargeCompute(AsyncValue *const *inputs, int numArguments,
                  RCReference<AsyncValue> *result, int numResult,
                  HostContext *ctx) {
  (void)numArguments;
  (void)numResult;
  auto &vec = inputs[0]->get<std::vector<int>>();
  result[0] = std::move(ctx->EnqueueWork([&vec]() {
    for (size_t i = 0, e = vec.size(); i < e; ++i) {
      vec[i] += 1;
    }
    return std::move(vec);
  }));
}

int SyncLargeCompute(std::vector<int> &input, std::vector<int> &output) {
  for (size_t i = 0, e = input.size(); i < e; ++i) {
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
  std::vector<RCReference<AsyncValue>> results;
  int numEle = 2000;
  results.resize(numEle);
  auto input1 =
      context->MakeAvailableAsyncValueRef<std::vector<int>>(100000, 0);

  std::vector<std::vector<int>> input;
  input.resize(numEle + 1);
  input[0] = std::vector<int>(100000, 0);

  auto start2 = Now();
  func->Execute(absl::MakeConstSpan({input1.GetAsyncValue()}),
                absl::MakeSpan(results.data(), 1), context.get());
  for (int i = 0; i < numEle - 1; ++i) {
    std::vector<AsyncValue *> mappedVec;
    mappedVec.push_back(results[i].get());
    func->Execute(mappedVec, absl::MakeSpan(results.data() + i + 1, 1),
                  context.get());
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
