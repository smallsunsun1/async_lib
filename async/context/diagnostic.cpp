#include "diagnostic.h"

#include "async/support/string_util.h"
#include "execution_context.h"
#include "host_context.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace sss {
namespace async {
DecodedDiagnostic::DecodedDiagnostic(const absl::Status& error) : message(error.message()) {}
std::ostream& operator<<(std::ostream& os, const DecodedDiagnostic& diag) {
  if (diag.location) {
    os << diag.location->filename << ":" << diag.location->line << ":" << diag.location->column << ": ";
  } else {
    os << "UnknownLocation: ";
  }
  return os << diag.message;
}
DecodedDiagnostic EmitError(const ExecutionContext& exec_ctx, absl::string_view message) {
  auto decoded_loc = exec_ctx.location().Decode();
  auto diag = DecodedDiagnostic(decoded_loc, message);
  auto* host = exec_ctx.host();
  host->EmitError(diag);
  return diag;
}

}  // namespace async
}  // namespace sss