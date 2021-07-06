#include <thread>

#include "async/trace/simple_tracing.h"

using namespace sss;
using namespace async;

int main() {
  Tracing *testTrace = SimpleTracing::GetTracing();
  testTrace->PushTracingScope("sss");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  testTrace->PopTracingScope("sss");
  testTrace->PushTracingScope("job1");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  testTrace->PushTracingScope("job2");
  testTrace->PopTracingScope("job2");
  testTrace->PopTracingScope("job1");

  ScopedTracing trace;
  // trace.PushTracingScope("scope1");
  trace.PushTracingScope("scope2");
  trace.PushTracingScope("scope1");

  return 0;
}
