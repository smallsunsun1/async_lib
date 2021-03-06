#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_TRACE_TRACING_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_TRACE_TRACING_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include "absl/status/status.h"

namespace sss {
namespace async {

enum class TracingLevel { Default = 0, Verbose = 1, Debug = 2 };
class Tracing {
 public:
  virtual ~Tracing() = default;
  virtual absl::Status RequestTracing(bool enable) = 0;
  virtual void RecordTracing(std::string name) = 0;
  virtual void PushTracingScope(std::string name) = 0;
  virtual void PopTracingScope(std::string name) = 0;
  static Tracing *GetTracing();
  static std::mutex *GetTracingMutex();
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_TRACE_TRACING_ */
