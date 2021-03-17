#ifndef ASYNC_RUNTIME_REGISTER_
#define ASYNC_RUNTIME_REGISTER_

#include <unordered_map>

#include "async_kernel.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace sss {
namespace async {

class KernelFnRegister {
 public:
  void InsertKernelFn(absl::string_view name, AsyncKernelFn fn);
  void RemoveKernelFn(absl::string_view name);
  absl::optional<AsyncKernelFn> GetKernelFn(absl::string_view name);

 private:
  absl::flat_hash_map<absl::string_view, AsyncKernelFn> mFuncLibs;
};

KernelFnRegister& GetKernelFnRegister() {
  static KernelFnRegister fnRegister;
  return fnRegister;
}

#define REGISTER_KERNEL_FN(name_, func) GetKernelFnRegister().InsertKernelFn(name_, func);
#define UNREGISTER_KERNEL_FN(name_) GetKernelFnRegister().RemoveKernelFn(name_);
#define GET_KERNEL_FN(name_) GetKernelFnRegister().GetKernelFn(name_);

}  // namespace async
}  // namespace sss

#endif /* ASYNC_RUNTIME_REGISTER_ */
