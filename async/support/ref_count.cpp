#include "ref_count.h"

namespace sss {
namespace async {
#ifndef NDEBUG
std::atomic<size_t> total_reference_counter_objects{0};
#endif

}  // namespace async
}  // namespace sss