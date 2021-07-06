#include "diagnostic.h"

#include <string_view>

#include "absl/status/status.h"
#include "async/context/execution_context.h"
#include "async/context/host_context.h"
#include "async/support/string_util.h"

namespace sss {
namespace async {
DecodedDiagnostic::DecodedDiagnostic(const absl::Status &error)
    : message(error.message()) {}
std::ostream &operator<<(std::ostream &os, const DecodedDiagnostic &diag) {
  if (diag.location) {
    os << diag.location->filename << ":" << diag.location->line << ":"
       << diag.location->column << ": ";
  } else {
    os << "UnknownLocation: ";
  }
  return os << diag.message;
}
DecodedDiagnostic EmitError(const ExecutionContext &exec_ctx,
                            std::string_view message) {
  auto decoded_loc = exec_ctx.location().Decode();
  auto diag = DecodedDiagnostic(decoded_loc, message);
  auto *host = exec_ctx.host();
  host->EmitError(diag);
  return diag;
}

}  // namespace async
}  // namespace sss