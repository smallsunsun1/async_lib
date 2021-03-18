#ifndef ASYNC_COROUTINE_SCHEDULE_
#define ASYNC_COROUTINE_SCHEDULE_

#include "task.h"
#include "coroutine_thread_pool.h"

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
auto ScheduleOn(Scheduler& scheduler, Awaitable&& awaitable) ->  {
    co_await scheduler.Schedule();
    co_return co_await std::forward<Awaitable>(awaitable);
}

template <typename Awaitable, typename Executor>
decltype(auto) operator|(Awaitable&& awaitable, SchedulerTransform<Executor>& scheduler) {
    return ScheduleOn(scheduler.mExec, std::forward<Awaitable>(awaitable));
}

}
}

#endif /* ASYNC_COROUTINE_SCHEDULE_ */
