#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_HANDLER_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_HANDLER_

#include <string>
#include <string_view>

#include "async/context/native_function.h"
#include "async/runtime/function_op.h"

namespace sss {
namespace async {

class BaseRuntime;

class FunctionHandler {
 public:
  explicit FunctionHandler(std::string_view name, BaseRuntime* runtime, FunctionHandler* handler) : mName(name), mRuntime(runtime), mFallback(handler) {}
  FunctionHandler* GetFallBackHandler() { return mFallback; }
  std::string_view GetName() const { return mName; }
  BaseRuntime* GetBaseRuntime() { return mRuntime; }
  absl::StatusOr<SimpleFunction> MakeFunction();

 private:
  const std::string mName;
  BaseRuntime* mRuntime;
  FunctionHandler* mFallback;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_HANDLER_ */
