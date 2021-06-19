#ifndef ASYNC_CONTEXT_DIAGNOSTIC_
#define ASYNC_CONTEXT_DIAGNOSTIC_

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "async/context/location.h"

namespace sss {
namespace async {

class ExecutionContext;
struct DecodedDiagnostic {
  explicit DecodedDiagnostic(const absl::Status& error);
  explicit DecodedDiagnostic(absl::string_view message) : message(message) {}
  DecodedDiagnostic(DecodedLocation location, absl::string_view message) : location(std::move(location)), message(message) {}
  absl::optional<DecodedLocation> location;
  std::string message;
};
std::ostream& operator<<(std::ostream& os, const DecodedDiagnostic& diagnostic);
DecodedDiagnostic EmitError(const ExecutionContext& exec_ctx, absl::string_view message);
template <typename... Args>
DecodedDiagnostic EmitError(const ExecutionContext& exec_ctx, Args&&... args) {
  return EmitError(exec_ctx, absl::string_view{absl::StrCat(std::forward<Args>(args)...)});
}

}  // namespace async
}  // namespace sss

#endif /* ASYNC_CONTEXT_DIAGNOSTIC_ */
