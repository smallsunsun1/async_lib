#include "async_value_ref.h"

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "async/context/diagnostic.h"
#include "async/context/execution_context.h"
#include "async/context/host_context.h"

namespace sss {
namespace async {

RCReference<ErrorAsyncValue> EmitErrorAsync(const ExecutionContext& execCtx, absl::string_view message) {
  auto diag = EmitError(execCtx, message);
  return execCtx.host()->MakeErrorAsyncValueRef(std::move(diag));
}
RCReference<ErrorAsyncValue> EmitErrorAsync(const ExecutionContext& execCtx, absl::Status error) { return EmitErrorAsync(execCtx, error.message()); }

}  // namespace async
}  // namespace sss