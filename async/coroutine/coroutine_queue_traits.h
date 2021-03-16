#ifndef ASYNC_COROUTINE_COROUTINE_QUEUE_TRAITS_
#define ASYNC_COROUTINE_COROUTINE_QUEUE_TRAITS_

#include "async/concurrent/work_queue_base.h"
#include "coroutine_task_deque.h"
#include "coroutine_task_queue.h"

namespace ficus {
namespace async {
namespace internal {

template <typename ThreadingEnvironment>
class NonBlockingCoroutineQueue;
template <typename ThreadingEnvironment>
class BlockingCoroutineQueue;

template <typename ThreadingEnvironmentTy>
struct WorkQueueTraits<NonBlockingCoroutineQueue<ThreadingEnvironmentTy>> {
  using ThreadingEnvironment = ThreadingEnvironmentTy;
  using Thread = typename ThreadingEnvironment::Thread;
  using Queue = typename ::ficus::async::internal::CoroutineTaskDeque;
};
template <typename ThreadingEnvironmentTy>
struct WorkQueueTraits<BlockingCoroutineQueue<ThreadingEnvironmentTy>> {
  using ThreadingEnvironment = ThreadingEnvironmentTy;
  using Thread = typename ThreadingEnvironment::Thread;
  using Queue = typename ::ficus::async::internal::CoroutineTaskQueue;
};

}  // namespace internal
}  // namespace async
}  // namespace ficus

#endif /* ASYNC_COROUTINE_COROUTINE_QUEUE_TRAITS_ */