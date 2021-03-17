#include "function.h"

namespace sss {
namespace async {

void Function::Execute(const Function* func, std::vector<RCReference<AsyncValue>>& arguments, std::vector<RCReference<AsyncValue>>& results, HostContext* ctx, bool blockedTask) {
  absl::InlinedVector<AsyncValue*, 4> argumentsPtr;
  argumentsPtr.reserve(arguments.size());
  for (auto& elem : arguments) {
    argumentsPtr.push_back(elem.get());
  }
  if (blockedTask) {
    ctx->EnqueueBlockingWork(
        [ctx, &argumentsPtr, &results, func]() { func->Execute(absl::MakeConstSpan(argumentsPtr.data(), argumentsPtr.size()), absl::MakeSpan(results.data(), results.size()), ctx); });
  } else {
    ctx->EnqueueWork([ctx, &argumentsPtr, &results, func]() { func->Execute(absl::MakeConstSpan(argumentsPtr.data(), argumentsPtr.size()), absl::MakeSpan(results.data(), results.size()), ctx); });
  }
}

}  // namespace async
}  // namespace sss