#ifndef ASYNC_COROUTINE_SCHEDULE_
#define ASYNC_COROUTINE_SCHEDULE_

#include "coroutine_thread_pool.h"
#include "internal/awaiter_traits.h"
#include "task.h"

namespace sss {
namespace async {

template <typename Executor>
struct SchedulerTransform {
  SchedulerTransform(Executor& exec) noexcept : mExec(&exec) {}
  Executor& mExec;
};

template <typename Executor>
SchedulerTransform<Executor> ScheduleOn(Executor& exec) {
  return SchedulerTransform<Executor>(exec);
}

template <typename Awaitable, typename Scheduler>
auto ScheduleOn(Scheduler& scheduler, Awaitable&& awaitable) -> Task<internal::remove_rvalue_reference_t<typename awaitable_traits<Awaitable>::await_result_t>> {
  co_await scheduler.Schedule();
  co_return co_await std::forward<Awaitable>(awaitable);
}

template <typename Awaitable, typename Executor>
decltype(auto) operator|(Awaitable&& awaitable, SchedulerTransform<Executor>& scheduler) {
  return ScheduleOn(scheduler.mExec, std::forward<Awaitable>(awaitable));
}

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_SCHEDULE_ */
