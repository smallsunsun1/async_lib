#ifndef ASYNC_CONTEXT_EXECUTION_CONTEXT_
#define ASYNC_CONTEXT_EXECUTION_CONTEXT_

#include "async/context/location.h"

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

#endif /* ASYNC_CONTEXT_EXECUTION_CONTEXT_ */
