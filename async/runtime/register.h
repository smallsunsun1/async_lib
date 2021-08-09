#ifndef ASYNC_RUNTIME_REGISTER_
#define ASYNC_RUNTIME_REGISTER_

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "async/runtime/async_kernel.h"

namespace sss {
namespace async {

class KernelFnRegister {
 public:
  void InsertKernelFn(const std::string& name, AsyncKernelFn fn);
  void RemoveKernelFn(const std::string& name);
  std::optional<AsyncKernelFn> GetKernelFn(const std::string& name);
  AsyncKernelFn MustGetKernelFn(const std::string& name);

 private:
  std::unordered_map<std::string, AsyncKernelFn> mFuncLibs;
};

inline KernelFnRegister& GetKernelFnRegister() {
  static KernelFnRegister fnRegister;
  return fnRegister;
}

#define REGISTER_KERNEL_FN(name_, func) \
  GetKernelFnRegister().InsertKernelFn(name_, func)
#define UNREGISTER_KERNEL_FN(name_) GetKernelFnRegister().RemoveKernelFn(name_)
#define GET_KERNEL_FN(name_) GetKernelFnRegister().GetKernelFn(name_)

#define ASYNC_STATIC_KERNEL_REGISTRATION(NAME, FUNC) \
  ASYNC_STATIC_KERNEL_REGISTRATION_(NAME, FUNC, __COUNTER__)
#define ASYNC_STATIC_KERNEL_REGISTRATION_(NAME, FUNC, N) \
  ASYNC_STATIC_KERNEL_REGISTRATION__(NAME, FUNC, N)
#define ASYNC_STATIC_KERNEL_REGISTRATION__(NAME, FUNC, N)    \
  static bool async_static_kernel_##N##_registered_ = []() { \
    GetKernelFnRegister().InsertKernelFn(NAME, FUNC);        \
    return true;                                             \
  }()

}  // namespace async
}  // namespace sss

#endif /* ASYNC_RUNTIME_REGISTER_ */
