#include "async/context/host_allocator.h"

#include <atomic>

#include "async/support/alloc.h"

namespace sss {
namespace async {

class MallocAllocator : public HostAllocator {
  void* AllocateBytes(size_t size, size_t alignment) override {
    if (alignment <= 8) return malloc(size);
    size = (size + alignment - 1) / alignment * alignment;
    return AlignedAlloc(alignment, size);
  }
  void DeallocateBytes(void* ptr, size_t size) override { free(ptr); }
};

void HostAllocator::VtableAnchor() {}
std::unique_ptr<HostAllocator> CreateMallocAllocator() { return std::make_unique<MallocAllocator>(); }

}  // namespace async
}  // namespace sss