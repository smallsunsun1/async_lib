#ifndef INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_ASYNC_KERNEL_
#define INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_ASYNC_KERNEL_

#include <atomic>
#include <functional>
#include <vector>

#include "async/support/function_ref.h"

namespace sss {

namespace async {
class CommonAsyncKernelFrame;
class AsyncValue;
}  // namespace async

// using AsyncKernelFn = void (*)(async::CommonAsyncKernelFrame*);
// using AsyncKernelFn =
// async::function_ref<void(async::CommonAsyncKernelFrame*)>;
using AsyncKernelFn = std::function<void(async::CommonAsyncKernelFrame *)>;

struct AsyncValueInfo {
  unsigned mUserCount;  // 有多少用户使用了这个AsyncValue
  std::atomic<async::AsyncValue *> mValue{nullptr};
  AsyncValueInfo() : mUserCount(0) {}
  AsyncValueInfo(AsyncValueInfo &&value) : mUserCount(value.mUserCount) {
    mValue.store(value.mValue.load());
  }
  explicit AsyncValueInfo(unsigned userCount) : mUserCount(userCount) {}
};
struct KernelResInfo {
  std::vector<std::vector<unsigned>>
      mResultTable;  // 对应于第几个kernel+第几个输出结果
};
struct KernelInfo {
  KernelInfo(KernelInfo &&value) {
    mArgumentsNotReady.store(value.mArgumentsNotReady.load());
  }
  std::atomic<int> mArgumentsNotReady;
  KernelInfo() = default;
  KernelInfo(unsigned numOperands)
      : mArgumentsNotReady(std::max(static_cast<unsigned>(1), numOperands)) {}
};
struct FunctionInfo {
  std::vector<AsyncValueInfo>
      mAsyncValueInfos;  // 存储整个Function调用过程中的对应AsyncValue和它的userCount
  std::vector<KernelInfo>
      mKernelInfos;  // 存储了对应Kernel中有多少Arguments还未ready
};

}  // namespace sss

#endif /* INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_ASYNC_KERNEL_ */
