#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_MEMORY_GPU_ALLOCATOR_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_MEMORY_GPU_ALLOCATOR_

#include <cstddef>

namespace sss {
namespace async {
namespace gpu {

class GpuAllocator {
 public:
  static constexpr size_t kAlignment = 256;
  virtual ~GpuAllocator() = default;
};

}  // namespace gpu
}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_MEMORY_GPU_ALLOCATOR_ \
        */
