#ifndef ASYNC_RUNTIME_REGISTER_
#define ASYNC_RUNTIME_REGISTER_

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "async/context/async_value.h"
#include "async/context/kernel_frame.h"
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

template <typename T, typename Ret, typename... Args, size_t... N>
auto GenWrappedLambdaImpl(Ret (T::*func)(Args...), T* cls,
                          std::index_sequence<N...> seq) {
  return [cls, func](CommonAsyncKernelFrame* kernelFrame) {
    return (cls->*func)(kernelFrame->GetArgAt<Args>(N)...);
  };
}

template <typename T, typename Ret, typename... Args>
auto GenWrappedLambda(Ret (T::*func)(Args...), T* cls) {
  return GenWrappedLambdaImpl(func, cls,
                              std::make_index_sequence<sizeof...(Args)>());
}

#define REGISTER_CLASS_KERNEL_FN(classMemberFuncPtr, classPtr) \
  GenWrappedLambda(classMemberFuncPtr, classPtr);

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
