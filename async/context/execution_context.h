#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_EXECUTION_CONTEXT_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_EXECUTION_CONTEXT_

#include "location.h"

namespace sss {
namespace async {
class HostContext;

class ExecutionContext {
 public:
  explicit ExecutionContext(HostContext* host) : mHost{host} {}

  Location location() const { return mLocation; }
  HostContext* host() const { return mHost; }

  void set_location(Location location) { mLocation = location; }

 private:
  Location mLocation;
  HostContext* mHost = nullptr;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_EXECUTION_CONTEXT_ */
