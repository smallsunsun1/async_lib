#include "async/context/function.h"  // for Function

#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult

#include <memory>   // for unique_ptr
#include <utility>  // for move

#include "absl/container/inlined_vector.h"  // for InlinedVector
#include "absl/memory/memory.h"             // for allocator_traits<>::value...
#include "absl/types/span.h"                // for MakeConstSpan, MakeSpan
#include "async/context/async_value.h"      // for AsyncValue
#include "async/context/async_value_ref.h"  // for AsyncValueRef
#include "async/context/host_context.h"     // for HostContext, CreateSimple...
#include "async/context/native_function.h"  // for SimpleFunction, NewFunction
#include "async/support/ref_count.h"        // for RCReference, TakeRef, async
#include "gtest/gtest_pred_impl.h"          // for Test, InitGoogleTest, RUN...

using namespace sss;
using namespace async;

void LargeCompute(AsyncValue* const* inputs, int numArguments, RCReference<AsyncValue>* result, int numResult, HostContext* ctx) {
  float a = inputs[0]->get<float>() + 100;
  result[0] = ctx->EnqueueWork([a]() { return a + 2; });
}

class ComputeTest {
 public:
  ComputeTest(HostContext* ctx) : mpContext(ctx) {
    mpFunc = TakeRef(NewFunction("ComputeTest::DoLargeCompute", [this](AsyncValue* const* inputs, int numArguments, RCReference<AsyncValue>* result, int numResult, HostContext* ctx) {
      float a = inputs[0]->get<float>();
      result[0] = ctx->EnqueueWork([this, a]() { return this->DoLargeCompute(a); });
    }));
  }
  void ExecuteCompute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results) const { mpFunc->Execute(arguments, results, mpContext); }
  float DoLargeCompute(float a) const { return a + 102; }

 private:
  HostContext* mpContext;
  RCReference<const Function> mpFunc;
};

TEST(ASYNCFUNCTION, V1) {
  auto context = CreateSimpleHostContext();
  RCReference<const SimpleFunction> func = TakeRef(new SimpleFunction("LargeCompute", LargeCompute));
  absl::InlinedVector<RCReference<AsyncValue>, 4> result1;
  absl::InlinedVector<RCReference<AsyncValue>, 4> result2;
  absl::InlinedVector<RCReference<AsyncValue>, 4> result3;
  result1.resize(1);
  result2.resize(1);
  result3.resize(1);
  auto input1 = context->MakeAvailableAsyncValueRef<float>(2);
  func->Execute(absl::MakeConstSpan({input1.GetAsyncValue()}), absl::MakeSpan(result1.data(), result1.size()), context.get());
  absl::InlinedVector<AsyncValue*, 4> mappedVec;
  for (int i = 0; i < result1.size(); ++i) {
    mappedVec.push_back(result1[i].get());
  }
  func->Execute(absl::MakeConstSpan(mappedVec.data(), mappedVec.size()), absl::MakeSpan(result2.data(), result2.size()), context.get());
  context->Await(result2);
  auto value1 = result2[0]->get<float>();
  // assert(result2[0]->get<float>() == 206 && "result must be 206, some error
  // happened!");
  EXPECT_EQ(value1, 206);
  ComputeTest compute(context.get());
  compute.ExecuteCompute(absl::MakeConstSpan(mappedVec.data(), mappedVec.size()), absl::MakeSpan(result3.data(), result3.size()));
  context->Await(result3);
  auto value2 = result3[0]->get<float>();
  EXPECT_EQ(value2, 206);
  // assert(result3[0]->get<float>() == 206 && "result must be 206, some error
  // happened!"); std::cout << "Test Passed!" << std::endl;
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
