#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_ENVIRONMENT_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_ENVIRONMENT_

#include <memory>
#include <thread>

namespace sss {
namespace async {
namespace internal {
struct StdThreadingEnvironment {
  using Thread = std::thread;
  template <typename Function, typename... Args>
  static std::unique_ptr<Thread> StartThread(Function &&f, Args &&... args) {
    return std::make_unique<Thread>(std::forward<Function>(f),
                                    std::forward<Args>(args)...);
  }
  static void Join(Thread *thread) { thread->join(); }
  static void Detatch(Thread *thread) { thread->detach(); }
  static uint64_t ThisThreadIdHash() {
    return std::hash<std::thread::id>()(std::this_thread::get_id());
  }
};

}  // namespace internal
}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_ENVIRONMENT_ */
