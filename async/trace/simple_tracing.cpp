#include "async/trace/simple_tracing.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>

namespace sss {
namespace async {
namespace {
const auto kProcessStart = std::chrono::steady_clock::now();
class TimeInfoUtility {
  using Time = std::chrono::steady_clock::time_point;
  struct TimeInfo {
    std::string name;
    Time start;
    Time end;
  };

 public:
  static constexpr size_t kMaxElement = 100;
  ~TimeInfoUtility() {
    for (const auto &timeInfo : mActivities) {
      auto startUs = std::chrono::duration_cast<std::chrono::microseconds>(
          timeInfo.start - kProcessStart);
      if (timeInfo.start == timeInfo.end) {
        std::cout << timeInfo.name << ": " << startUs.count() << "us"
                  << "\n";
        continue;
      }
      auto endUs = std::chrono::duration_cast<std::chrono::microseconds>(
          timeInfo.end - kProcessStart);
      auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(
          timeInfo.end - timeInfo.start);
      std::cout << timeInfo.name << ": "
                << "start us: " << startUs.count()
                << " end us : " << endUs.count() << ": "
                << "duration : " << durationUs.count() << " us"
                << "\n";
    }
    std::cout << "Thread Id: " << std::this_thread::get_id() << " ,  "
              << "Total TimeInfo Number Is: " << mActivities.size() << "\n";
  }
  void RecordEvent(std::string name) {
    if (mActivities.size() >= kMaxElement) return;
    auto timeNow = Now();
    mActivities.push_back(TimeInfo{std::move(name), timeNow, timeNow});
  }
  void PushScope(std::string name) {
    if (mActivities.size() >= kMaxElement) return;
    mStack.emplace_back(std::move(name), Now());
  }
  void PopScope() {
    if (mActivities.size() >= kMaxElement) return;
    if (mStack.empty()) return;
    mActivities.push_back({std::move(std::get<0>(mStack.back())),
                           std::get<1>(mStack.back()), Now()});
    mStack.pop_back();
  }

 private:
  static Time Now() { return std::chrono::steady_clock::now(); }
  std::vector<std::tuple<std::string, Time>> mStack;
  std::vector<TimeInfo> mActivities;
};

}  // namespace

namespace internal {

void ScopeTimeInfoUtility::ReleaseInfo() {
  auto startUs = std::chrono::duration_cast<std::chrono::microseconds>(
      mTime.start - kProcessStart);
  if (mTime.start == mTime.end) {
    std::cout << mTime.name << ": " << startUs.count() << "us\n";
  } else {
    auto time = ScopeTimeInfoUtility::Now();
    mTime.end = std::move(time);
    auto endUs = std::chrono::duration_cast<std::chrono::microseconds>(
        mTime.end - kProcessStart);
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(
        mTime.end - mTime.start);
    std::cout << mTime.name << ": "
              << "start us: " << startUs.count()
              << " end us : " << endUs.count() << ": "
              << "duration : " << durationUs.count() << " us\n";
    std::cout << "Thread Id: " << std::this_thread::get_id() << "\n";
  }
}

}  // namespace internal

static TimeInfoUtility &GetTimeInfoUtility() {
  static thread_local TimeInfoUtility timeInfoUtility;
  return timeInfoUtility;
}

Tracing *SimpleTracing::GetTracing() {
  static Tracing *res = new SimpleTracing;
  return res;
}

absl::Status SimpleTracing::RequestTracing(bool enable) {
  (void)enable;
  return absl::OkStatus();
}
void SimpleTracing::RecordTracing(std::string name) {
  GetTimeInfoUtility().RecordEvent(std::move(name));
}
void SimpleTracing::PushTracingScope(std::string name) {
  GetTimeInfoUtility().PushScope(std::move(name));
}
void SimpleTracing::PopTracingScope(std::string name) {
  (void)name;
  GetTimeInfoUtility().PopScope();
}

absl::Status ScopeTracing::RequestTracing(bool enable) {
  (void)enable;
  return absl::OkStatus();
}

// ScopeTracing Only Use in Function Scope, So different thread must have
// different Tracing object
Tracing *ScopeTracing::GetTracing() {
  static thread_local Tracing *res = new ScopeTracing();
  return res;
}

void ScopeTracing::PushTracingScope(std::string name) {
  mTimeInfos.emplace_back(std::move(name),
                          internal::ScopeTimeInfoUtility::Now());
}

void ScopeTracing::PopTracingScope(std::string name) {
  (void)name;
  mTimeInfos.back().ReleaseInfo();
  mTimeInfos.pop_back();
}

void ScopeTracing::RecordTracing(std::string name) {
  auto timeNow = internal::ScopeTimeInfoUtility::Now();
  mTimeInfos.emplace_back(std::move(name), timeNow, timeNow);
}

absl::Status ScopedTracing::RequestTracing(bool enable) {
  (void)enable;
  return absl::OkStatus();
}

void ScopedTracing::PushTracingScope(std::string name) {
  mTimeInfos.emplace_back(std::move(name),
                          internal::ScopeTimeInfoUtility::Now());
}

void ScopedTracing::PopTracingScope(std::string name) {
  (void)name;
  mTimeInfos.back().ReleaseInfo();
  mTimeInfos.pop_back();
}

void ScopedTracing::RecordTracing(std::string name) {
  auto timeNow = internal::ScopeTimeInfoUtility::Now();
  mTimeInfos.emplace_back(std::move(name), timeNow, timeNow);
}

ScopedTracing::~ScopedTracing() {
  while (!mTimeInfos.empty()) {
    mTimeInfos.back().ReleaseInfo();
    mTimeInfos.pop_back();
  }
}

}  // namespace async
}  // namespace sss
