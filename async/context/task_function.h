#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_TASK_FUNCTION_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_TASK_FUNCTION_

#include "async/support/unique_function.h"

namespace ficus {
namespace async {

class TaskFunction {
 public:
  explicit TaskFunction(unique_function<void()> work) : mFunc(std::move(work)) {}
  TaskFunction(TaskFunction&&) = default;
  TaskFunction() = default;
  void operator()() { mFunc(); }
  TaskFunction& operator=(TaskFunction&& f) = default;
  void reset() { mFunc = nullptr; }
  explicit operator bool() const { return static_cast<bool>(mFunc); }

 private:
  TaskFunction(const TaskFunction&) = delete;
  TaskFunction& operator=(const TaskFunction&) = delete;
  unique_function<void()> mFunc;
};

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_TASK_FUNCTION_ */
