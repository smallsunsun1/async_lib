#include "async/runtime/register.h"

#include <cassert>

namespace sss {
namespace async {

void KernelFnRegister::InsertKernelFn(const std::string& name,
                                      AsyncKernelFn fn) {
  if (mFuncLibs.find(name) != mFuncLibs.end()) {
    assert(false && "Current Function Already Exist!");
  }
  mFuncLibs.insert({name, std::move(fn)});
}

void KernelFnRegister::RemoveKernelFn(const std::string& name) {
  auto iter = mFuncLibs.find(name);
  if (iter != mFuncLibs.end()) {
    mFuncLibs.erase(iter);
  }
}

std::optional<AsyncKernelFn> KernelFnRegister::GetKernelFn(
    const std::string& name) {
  if (mFuncLibs.find(name) == mFuncLibs.end()) {
    return std::nullopt;
  }
  return std::optional<AsyncKernelFn>(mFuncLibs[name]);
}

AsyncKernelFn KernelFnRegister::MustGetKernelFn(const std::string& name) {
  assert(mFuncLibs.find(name) != mFuncLibs.end() && "kernel fn must be found");
  return mFuncLibs[name];
}

}  // namespace async
}  // namespace sss