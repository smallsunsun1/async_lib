#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_ALLOCATOR_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_ALLOCATOR_

#include <memory>

#include "absl/types/span.h"

namespace sss {
namespace async {
class HostAllocator {
 public:
 public:
  virtual ~HostAllocator() = default;

  // Allocate memory for one or more entries of type T.
  template <typename T>
  T* Allocate(size_t num_elements = 1) {
    return static_cast<T*>(AllocateBytes(sizeof(T) * num_elements, alignof(T)));
  }

  // Deallocate the memory for one or more entries of type T.
  template <typename T>
  void Deallocate(T* ptr, size_t num_elements = 1) {
    DeallocateBytes(ptr, sizeof(T) * num_elements);
  }

  // Allocate the specified number of bytes with the specified alignment.
  virtual void* AllocateBytes(size_t size, size_t alignment) = 0;

  // Deallocate the specified pointer that has the specified size.
  virtual void DeallocateBytes(void* ptr, size_t size) = 0;

 protected:
  friend class HostContext;
  friend class FixedSizeAllocator;
  friend class ProfiledAllocator;
  HostAllocator() = default;
  HostAllocator(const HostAllocator&) = delete;
  HostAllocator& operator=(const HostAllocator&) = delete;

 private:
  virtual void VtableAnchor();
};

std::unique_ptr<HostAllocator> CreateMallocAllocator();
std::unique_ptr<HostAllocator> CreateFixedSizeAllocator(size_t capacity = 1024);
std::unique_ptr<HostAllocator> CreateProfiledAllocator(std::unique_ptr<HostAllocator> allocator);
std::unique_ptr<HostAllocator> CreateLeakCheckAllocator(std::unique_ptr<HostAllocator> allocator);
template <typename ObjectT>
class HostArray {
 public:
  HostArray() {}
  HostArray(size_t num_objects, HostAllocator* host_allocator) : mArray(absl::Span<ObjectT>(host_allocator->Allocate<ObjectT>(num_objects), num_objects)), mHostAllocator(host_allocator) {}

  ~HostArray() {
    // Destroy all of the objects.
    for (auto& object : mArray) object.~ObjectT();
    if (mHostAllocator != nullptr) mHostAllocator->Deallocate(mArray.data(), mArray.size());
  }

  // HostArray is move-only.
  HostArray(HostArray&& other) : mArray(other.mArray), mHostAllocator(other.mHostAllocator) {
    other.mArray = {};
    other.mHostAllocator = nullptr;
  }
  HostArray& operator=(HostArray&& other) {
    mArray = other.mArray;
    other.mArray = {};

    mHostAllocator = other.mHostAllocator;
    other.mHostAllocator = nullptr;
    return *this;
  }
  HostArray(const HostArray&) = delete;
  HostArray& operator=(const HostArray&) = delete;

  absl::Span<const ObjectT> array() const { return mArray; }
  absl::Span<ObjectT>& mutable_array() { return mArray; }
  size_t size() const { return mArray.size(); }
  ObjectT& operator[](size_t index) { return mArray[index]; }

 private:
  absl::Span<ObjectT> mArray;
  HostAllocator* mHostAllocator = nullptr;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_ALLOCATOR_ */
