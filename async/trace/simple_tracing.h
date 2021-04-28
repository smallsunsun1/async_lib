#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_TRACE_SIMPLE_TRACING_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_TRACE_SIMPLE_TRACING_

#include "third_party/abseil-cpp/absl/container/inlined_vector.h"
#include "tracing.h"

namespace sss {
namespace async {

class SimpleTracing : public Tracing {
 public:
  static Tracing* GetTracing();
  absl::Status RequestTracing(bool enable) override;
  void RecordTracing(std::string name) override;
  void PushTracingScope(std::string name) override;
  void PopTracingScope(std::string name) override;
};

namespace internal {
class ScopeTimeInfoUtility {
  using Time = std::chrono::steady_clock::time_point;
  struct TimeInfo {
    std::string name;
    Time start;
    Time end;
  };
  TimeInfo mTime;

 public:
  ScopeTimeInfoUtility(std::string name, Time now) : mTime{std::move(name), std::move(now)} {}
  ScopeTimeInfoUtility(std::string name, Time start, Time end) : mTime{std::move(name), std::move(start), std::move(end)} {}
  static Time Now() { return std::chrono::steady_clock::now(); }
  void ReleaseInfo();
};
}  // namespace internal

class ScopeTracing : public Tracing {
 public:
  static Tracing* GetTracing();
  absl::Status RequestTracing(bool enable) override;
  void RecordTracing(std::string name) override;
  void PushTracingScope(std::string name) override;
  void PopTracingScope(std::string name) override;
  virtual ~ScopeTracing() {}

 private:
  absl::InlinedVector<internal::ScopeTimeInfoUtility, 4> mTimeInfos;
};

class ScopedTracing : public Tracing {
 public:
  absl::Status RequestTracing(bool enable) override;
  void RecordTracing(std::string name) override;
  void PushTracingScope(std::string name) override;
  void PopTracingScope(std::string name) override;
  virtual ~ScopedTracing();

 private:
  std::vector<internal::ScopeTimeInfoUtility> mTimeInfos;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_TRACE_SIMPLE_TRACING_ */
