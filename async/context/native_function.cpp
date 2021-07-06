#include "async/context/native_function.h"

namespace sss {
namespace async {

void SimpleFunction::Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const {
  std::vector<AsyncValue*> unavailableArgs;
  for (auto* av : arguments) {
    if (!av->IsAvailable()) unavailableArgs.push_back(av);
  }
  if (unavailableArgs.empty()) {
    mCallable(arguments.data(), arguments.size(), results.data(), results.size(), host);
    return;
  }
  std::vector<RCReference<AsyncValue>> args;
  args.reserve(arguments.size());
  for (auto* av : arguments) args.push_back(FormRef(av));
  std::vector<RCReference<IndirectAsyncValue>> indirectResults;
  indirectResults.reserve(results.size());
  for (auto& avRef : results) {
    indirectResults.push_back(host->MakeIndirectAsyncValue());
    avRef = indirectResults.back().CopyRef();
  }
  host->RunWhenReady(unavailableArgs, [this, host, args = std::move(args), indirectResults = std::move(indirectResults)]() mutable {
    std::vector<AsyncValue*> argAvs;
    argAvs.reserve(args.size());
    for (const auto& arg : args) argAvs.push_back(arg.get());
    std::vector<RCReference<AsyncValue>> results;
    results.resize(indirectResults.size());
    mCallable(argAvs.data(), argAvs.size(), results.data(), results.size(), host);
    for (int i = 0, e = results.size(); i != e; ++i) {
      assert(results[i]);
      indirectResults[i]->ForwardTo(std::move(results[i]));
    }
  });
}

}  // namespace async
}  // namespace sss
