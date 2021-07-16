#include "async_value_ref.h"

#include <string_view>

#include "async/context/diagnostic.h"
#include "async/context/execution_context.h"
#include "async/context/host_context.h"

namespace sss {
namespace async {

RCReference<ErrorAsyncValue> EmitErrorAsync(const ExecutionContext &execCtx,
                                            std::string_view message) {
  auto diag = EmitError(execCtx, message);
  return execCtx.host()->MakeErrorAsyncValueRef(std::move(diag));
}

}  // namespace async
}  // namespace sss