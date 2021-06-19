#include "async/runtime/register.h"

namespace sss {
namespace async {

void KernelFnRegister::InsertKernelFn(absl::string_view name, AsyncKernelFn fn) {
  if (mFuncLibs.find(name) != mFuncLibs.end()) {
    assert(false && "Current Function Already Exist!");
  }
  mFuncLibs.insert({name, std::move(fn)});
}

void KernelFnRegister::RemoveKernelFn(absl::string_view name) {
  auto iter = mFuncLibs.find(name);
  if (iter != mFuncLibs.end()) {
    mFuncLibs.erase(iter);
  }
}

absl::optional<AsyncKernelFn> KernelFnRegister::GetKernelFn(absl::string_view name) {
  if (mFuncLibs.find(name) == mFuncLibs.end()) {
    return absl::nullopt;
  }
  return absl::optional<AsyncKernelFn>(mFuncLibs[name]);
}

}  // namespace async
}  // namespace sss