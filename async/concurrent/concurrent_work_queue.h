#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_CONCURRENT_WORK_QUEUE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_CONCURRENT_WORK_QUEUE_

#include <memory>
#include <optional>
#include <string_view>

#include "absl/types/span.h"
#include "async/support/ref_count.h"
#include "async/support/task_function.h"

namespace sss {
namespace async {
class AsyncValue;
class ConcurrentWorkQueue {
 public:
  virtual ~ConcurrentWorkQueue();
  virtual std::string name() const = 0;

 protected:
  virtual void AddTask(TaskFunction work) = 0;
  virtual std::optional<TaskFunction> AddBlockingTask(TaskFunction work,
                                                      bool allowQueuing) = 0;
  virtual void Await(absl::Span<const RCReference<AsyncValue>> values) = 0;
  virtual void Quiesce() = 0;
  virtual int GetParallelismLevel() const = 0;
  virtual bool IsInWorkerThread() const = 0;
  ConcurrentWorkQueue() = default;

 private:
  friend class HostContext;
  ConcurrentWorkQueue(const ConcurrentWorkQueue &) = delete;
  ConcurrentWorkQueue &operator=(const ConcurrentWorkQueue &) = delete;
};
std::unique_ptr<ConcurrentWorkQueue> CreateSingleThreadedWorkQueue();
std::unique_ptr<ConcurrentWorkQueue> CreateMultiThreadedWorkQueue(
    int num_threads, int num_blocking_threads);
using WorkQueueFactory =
    unique_function<std::unique_ptr<ConcurrentWorkQueue>(std::string_view arg)>;
std::unique_ptr<ConcurrentWorkQueue> CreateWorkQueue(std::string_view config);

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONCURRENT_CONCURRENT_WORK_QUEUE_ \
        */
