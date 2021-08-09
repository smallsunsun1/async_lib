#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_BUFFER_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_BUFFER_

#include <functional>

#include "async/support/ref_count.h"

namespace sss {
namespace async {

class HostAllocator;
class HostBuffer : public ReferenceCounted<HostBuffer> {
 public:
  using Deallocator = std::function<void(void *ptr, size_t size)>;
  static RCReference<HostBuffer> CreateUninitialized(size_t size,
                                                     size_t alignment,
                                                     HostAllocator *allocator);
  static RCReference<HostBuffer> CreateFromExternal(void *ptr, size_t size,
                                                    Deallocator deallocator);
  void *data() {
    if (mIsLined) return &mInlined.data[0];
    return mOutOfLine.ptr;
  }
  const void *data() const {
    if (mIsLined) return &mInlined.data[0];
    return mOutOfLine.ptr;
  }
  size_t size() const { return mSize; }

 private:
  friend class ReferenceCounted<HostBuffer>;
  HostBuffer(size_t size, HostAllocator *allocator)
      : mSize(size), mIsLined(true) {
    mInlined.allocator = allocator;
  }
  HostBuffer(void *ptr, size_t size, Deallocator deallocator)
      : mSize(size),
        mIsLined(false) {
    mOutOfLine.ptr = ptr;
    mOutOfLine.deallocator = std::move(deallocator);
  }
  ~HostBuffer();
  void Destroy();
  size_t mSize : 63;
  bool mIsLined : 1;
  union {
    struct {
      HostAllocator *allocator;
      alignas(alignof(std::max_align_t)) char data[];
    } mInlined;
    struct {
      void *ptr;
      Deallocator deallocator;
    } mOutOfLine;
  };
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_BUFFER_ */
