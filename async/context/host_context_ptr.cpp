#include "host_context_ptr.h"

#include "host_context.h"

namespace ficus {
namespace async {
HostContextPtr::HostContextPtr(HostContext* host) : HostContextPtr{host->instance_ptr()} {}

HostContext* HostContextPtr::get() const {
  assert(mIndex != kDummyIndex);
  return HostContext::GetHostContextByIndex(mIndex);
}

}  // namespace async
}  // namespace ficus