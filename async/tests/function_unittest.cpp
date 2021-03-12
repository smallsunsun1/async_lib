#include <functional>
#include <iostream>
#include <thread>
#include <utility>

#include "../context/host_context.h"
#include "../context/native_function.h"
#include "gtest/gtest.h"

using namespace ficus;
using namespace async;

void LargeCompute(AsyncValue* const* inputs, int numArguments,
                  RCReference<AsyncValue>* result, int numResult,
                  HostContext* ctx) {
  float a = inputs[0]->get<float>() + 100;
  result[0] = ctx->EnqueueWork([a]() { return a + 2; });
}

class ComputeTest {
 public:
  ComputeTest(HostContext* ctx) : mContext(ctx) {
    mpFunc =
        TakeRef(NewFunction("ComputeTest::DoLargeCompute",
                            [this](AsyncValue* const* inputs, int numArguments,
                                   RCReference<AsyncValue>* result,
                                   int numResult, HostContext* ctx) {
                              float a = inputs[0]->get<float>();
                              result[0] = ctx->EnqueueWork([this, a]() {
                                return this->DoLargeCompute(a);
                              });
                            }));
  }
  void ExecuteCompute(
      llvm::ArrayRef<AsyncValue*> arguments,
      llvm::MutableArrayRef<RCReference<AsyncValue>> results) const {
    mpFunc->Execute(arguments, results, mContext);
  }
  float DoLargeCompute(float a) const { return a + 102; }

 private:
  HostContext* mContext;
  RCReference<const Function> mpFunc;
};

TEST(ASYNCFUNCTION, V1) {
  auto context = CreateSimpleHostContext();
  RCReference<const SimpleFunction> func =
      TakeRef(new SimpleFunction("LargeCompute", LargeCompute));
  llvm::SmallVector<RCReference<AsyncValue>, 4> result1;
  llvm::SmallVector<RCReference<AsyncValue>, 4> result2;
  llvm::SmallVector<RCReference<AsyncValue>, 4> result3;
  result1.resize(1);
  result2.resize(1);
  result3.resize(1);
  auto input1 = context->MakeAvailableAsyncValueRef<float>(2);
  func->Execute(llvm::ArrayRef<AsyncValue*>{input1.GetAsyncValue()}, result1,
                context.get());
  auto mapped = llvm::map_range(
      result1, [](RCReference<AsyncValue>& input) { return input.get(); });
  llvm::SmallVector<AsyncValue*, 4> mappedVec(mapped.begin(), mapped.end());
  func->Execute(mappedVec, result2, context.get());
  context->Await(result2);
  auto value1 = result2[0]->get<float>();
  // assert(result2[0]->get<float>() == 206 && "result must be 206, some error
  // happened!");
  EXPECT_EQ(value1, 206);
  ComputeTest compute(context.get());
  compute.ExecuteCompute(mappedVec, result3);
  context->Await(result3);
  auto value2 = result3[0]->get<float>();
  EXPECT_EQ(value2, 206);
  // assert(result3[0]->get<float>() == 206 && "result must be 206, some error
  // happened!"); std::cout << "Test Passed!" << std::endl;
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  (void)RUN_ALL_TESTS();
  return 0;
}
