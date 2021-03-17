#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_BASE_RUNTIME_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_BASE_RUNTIME_

#include <memory>

namespace sss {
namespace async {

class HostContext;
class AsyncValue;
class SimpleFunction;
class HostAllocator;
class Chain;

class BaseRuntime {
 public:
 private:
  friend class FunctionHandler;
  class RuntimeImpl;
  std::unique_ptr<RuntimeImpl> mImpl;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_BASE_RUNTIME_ */
