#include "ref_count.h"

namespace ficus {
namespace async {
#ifndef NDEBUG
std::atomic<size_t> total_reference_counter_objects{0};
#endif

}  // namespace async
}  // namespace ficus