#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_ALLOC_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_ALLOC_

#include <cstddef>

namespace sss {
namespace async {
void* AlignedAlloc(size_t alignment, size_t size);

}
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_ALLOC_ */
