#include "async/context/host_buffer.h"

#include "async/context/host_allocator.h"

namespace sss {
namespace async {

RCReference<HostBuffer> HostBuffer::CreateUninitialized(size_t size, size_t alignment, HostAllocator* allocator) {
  assert(alignof(std::max_align_t) >= alignment && "Invalid alignment");
  auto* buf = allocator->AllocateBytes(sizeof(HostBuffer) + size, alignof(HostBuffer));
  if (!buf) return {};
  return TakeRef(new (buf) HostBuffer(size, allocator));
}
RCReference<HostBuffer> HostBuffer::CreateFromExternal(void* ptr, size_t size, Deallocator deallocator) { return TakeRef(new HostBuffer(ptr, size, std::move(deallocator))); }
HostBuffer::~HostBuffer() {
  if (!mIsLined) {
    mOutOfLine.deallocator(mOutOfLine.ptr, mSize);
    mOutOfLine.deallocator.~Deallocator();
  }
}
void HostBuffer::Destroy() {
  if (mIsLined) {
    this->~HostBuffer();
    mInlined.allocator->DeallocateBytes(this, sizeof(HostBuffer) + mSize);
  } else {
    delete this;
  }
}

}  // namespace async
}  // namespace sss