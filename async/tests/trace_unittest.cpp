#include <thread>

#include "../trace/simple_tracing.h"

using namespace ficus;
using namespace async;

int main() {
  Tracing* testTrace = SimpleTracing::GetTracing();
  testTrace->PushTracingScope("sss");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  testTrace->PopTracingScope("sss");
  testTrace->PushTracingScope("job1");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  testTrace->PushTracingScope("job2");
  testTrace->PopTracingScope("job2");
  testTrace->PopTracingScope("job1");
  return 0;
}
