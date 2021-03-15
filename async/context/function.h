#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_FUNCTION_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_FUNCTION_

#include "async/support/ref_count.h"
#include "third_party/abseil-cpp/absl/container/inlined_vector.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"
#include "third_party/abseil-cpp/absl/types/span.h"

namespace ficus {
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

  absl::string_view name() const { return mName; }

  // Execute this function on the specified HostContext, passing the specified
  // arguments. This returns one AsyncValue for each result.
  virtual void Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const = 0;

  // Reference counting operations, used by async kernels to keep the underlying
  // storage for a function alive.
  virtual void AddRef() const = 0;
  virtual void DropRef() const = 0;
  Function(absl::string_view name) : mName(name) {}

  Function(Function&& other) = default;

 private:
  virtual void VtableAnchor();
  // This is the name of the function, or empty if anonymous.
  absl::string_view mName;
};

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_FUNCTION_ */
