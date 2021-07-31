#ifndef ASYNC_TRACE_TRACING_MANAGER_
#define ASYNC_TRACE_TRACING_MANAGER_

#include <unordered_map>
#include <string>
#include <vector>

namespace sss {
namespace async {

struct TimeInfo {
    std::vector<uint64_t> start_times;
    std::vector<uint64_t> end_times;
    uint64_t total_time;
};

// 用于全局profile,会保存运行过程中所有被trace数据的
// 总执行时间和单步执行时间，超出上限后自动dump
class TracingManager {
public:
private:
    std::unordered_map<std::string, TimeInfo> time_infos_;
};

}
}

#endif /* ASYNC_TRACE_TRACING_MANAGER_ */
