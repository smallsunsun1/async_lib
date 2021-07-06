#include "async/runtime/register.h"

namespace sss {
namespace async {

void KernelFnRegister::InsertKernelFn(std::string_view name, AsyncKernelFn fn) {
  if (mFuncLibs.find(name) != mFuncLibs.end()) {
    assert(false && "Current Function Already Exist!");
  }
  mFuncLibs.insert({name, std::move(fn)});
}

void KernelFnRegister::RemoveKernelFn(std::string_view name) {
  auto iter = mFuncLibs.find(name);
  if (iter != mFuncLibs.end()) {
    mFuncLibs.erase(iter);
  }
}

std::optional<AsyncKernelFn> KernelFnRegister::GetKernelFn(
    std::string_view name) {
  if (mFuncLibs.find(name) == mFuncLibs.end()) {
    return std::nullopt;
  }
  return std::optional<AsyncKernelFn>(mFuncLibs[name]);
}

}  // namespace async
}  // namespace sss