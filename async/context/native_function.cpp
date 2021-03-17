#include "native_function.h"

namespace sss {
namespace async {

void SimpleFunction::Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const {
  absl::InlinedVector<AsyncValue*, 4> unavailableArgs;
  for (auto* av : arguments) {
    if (!av->IsAvailable()) unavailableArgs.push_back(av);
  }
  if (unavailableArgs.empty()) {
    mCallable(arguments.data(), arguments.size(), results.data(), results.size(), host);
    return;
  }
  absl::InlinedVector<RCReference<AsyncValue>, 4> args;
  args.reserve(arguments.size());
  for (auto* av : arguments) args.push_back(FormRef(av));
  absl::InlinedVector<RCReference<IndirectAsyncValue>, 4> indirectResults;
  indirectResults.reserve(results.size());
  for (auto& avRef : results) {
    indirectResults.push_back(host->MakeIndirectAsyncValue());
    avRef = indirectResults.back().CopyRef();
  }
  host->RunWhenReady(unavailableArgs, [this, host, args = std::move(args), indirectResults = std::move(indirectResults)]() mutable {
    absl::InlinedVector<AsyncValue*, 4> argAvs;
    argAvs.reserve(args.size());
    for (const auto& arg : args) argAvs.push_back(arg.get());
    absl::InlinedVector<RCReference<AsyncValue>, 4> results;
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
