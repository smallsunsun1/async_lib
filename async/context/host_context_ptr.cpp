#include "async/context/host_context_ptr.h"

#include "async/context/host_context.h"

namespace sss {
namespace async {
HostContextPtr::HostContextPtr(HostContext* host) : HostContextPtr{host->instance_ptr()} {}

HostContext* HostContextPtr::get() const {
  assert(mIndex != kDummyIndex);
  return HostContext::GetHostContextByIndex(mIndex);
}

}  // namespace async
}  // namespace sss