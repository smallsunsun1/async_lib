#include "async_value_ref.h"

#include "diagnostic.h"
#include "execution_context.h"
#include "host_context.h"
#include "third_party/abseil-cpp/absl/status/status.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"

namespace ficus {
namespace async {

RCReference<ErrorAsyncValue> EmitErrorAsync(const ExecutionContext& execCtx, absl::string_view message) {
  auto diag = EmitError(execCtx, message);
  return execCtx.host()->MakeErrorAsyncValueRef(std::move(diag));
}
RCReference<ErrorAsyncValue> EmitErrorAsync(const ExecutionContext& execCtx, absl::Status error) { return EmitErrorAsync(execCtx, error.message()); }

}  // namespace async
}  // namespace ficus