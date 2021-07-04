#ifndef ASYNC_RUNTIME_REGISTER_
#define ASYNC_RUNTIME_REGISTER_

#include <optional>
#include <string_view>
#include <unordered_map>

#include "async/runtime/async_kernel.h"

namespace sss {
namespace async {

class KernelFnRegister {
 public:
  void InsertKernelFn(std::string_view name, AsyncKernelFn fn);
  void RemoveKernelFn(std::string_view name);
  std::optional<AsyncKernelFn> GetKernelFn(std::string_view name);

 private:
  std::unordered_map<std::string_view, AsyncKernelFn> mFuncLibs;
};

inline KernelFnRegister& GetKernelFnRegister() {
  static KernelFnRegister fnRegister;
  return fnRegister;
}

#define REGISTER_KERNEL_FN(name_, func) GetKernelFnRegister().InsertKernelFn(name_, func)
#define UNREGISTER_KERNEL_FN(name_) GetKernelFnRegister().RemoveKernelFn(name_)
#define GET_KERNEL_FN(name_) GetKernelFnRegister().GetKernelFn(name_)

}  // namespace async
}  // namespace sss

#endif /* ASYNC_RUNTIME_REGISTER_ */
