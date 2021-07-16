#include "async/trace/tracing.h"

#include <cassert>

namespace sss {
namespace async {

std::mutex *Tracing::GetTracingMutex() {
  static std::mutex *mut = new std::mutex;
  return mut;
}

Tracing *Tracing::GetTracing() {
  assert(false && "Base Tracing Class is abstract and should not be called");
  return nullptr;
}

}  // namespace async
}  // namespace sss