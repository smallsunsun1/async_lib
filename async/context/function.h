#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_FUNCTION_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_FUNCTION_

#include <string_view>
#include <vector>

#include "absl/types/span.h"
#include "async/support/ref_count.h"

namespace sss {
namespace async {

class AsyncValue;
class HostContext;
template <typename T>
class RCReference;

class Function {
 public:
  Function() = default;
  virtual ~Function() {}
  Function(const Function&) = delete;
  Function& operator=(const Function&) = delete;

  std::string_view name() const { return mName; }

  // Execute this function on the specified HostContext, passing the specified
  // arguments. This returns one AsyncValue for each result.
  virtual void Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const = 0;
  static void Execute(const Function* func, std::vector<RCReference<AsyncValue>>& arguments, std::vector<RCReference<AsyncValue>>& results, HostContext* ctx, bool blockedTask = false);
  // Reference counting operations, used by async kernels to keep the underlying
  // storage for a function alive.
  virtual void AddRef() const = 0;
  virtual void DropRef() const = 0;
  Function(std::string_view name) : mName(name) {}

  Function(Function&& other) = default;

 private:
  virtual void VtableAnchor();
  // This is the name of the function, or empty if anonymous.
  std::string_view mName;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_FUNCTION_ */
