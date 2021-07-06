#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_REF_COUNT_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_REF_COUNT_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace sss {
namespace async {
#ifndef NDEBUG
extern std::atomic<size_t> total_reference_counter_objects;
inline size_t GetNumReferenceCountedObjects() {
  return total_reference_counter_objects.load(std::memory_order_relaxed);
}
inline void AddNumReferenceCountedObjects() {
  total_reference_counter_objects.fetch_add(1, std::memory_order_relaxed);
}
inline void DropNumReferenceCountedObjects() {
  total_reference_counter_objects.fetch_sub(1, std::memory_order_relaxed);
}
#else
inline void AddNumReferenceCountedObjects() {}
inline void DropNumReferenceCountedObjects() {}
#endif
template <typename SubClass>
class ReferenceCounted {
 public:
  ReferenceCounted() : mRefCounted(1) { AddNumReferenceCountedObjects(); }
  ~ReferenceCounted() {
    assert(mRefCounted.load() == 0 &&
           "Shouldn't destroy a reference counted object with references!");
    DropNumReferenceCountedObjects();
  }
  // 不能拷贝
  ReferenceCounted(const ReferenceCounted &) = delete;
  ReferenceCounted &operator=(const ReferenceCounted &) = delete;
  void AddRef() { mRefCounted.fetch_add(1, std::memory_order_relaxed); }
  void DropRef() {
    if (mRefCounted.fetch_sub(1, std::memory_order_acq_rel) == 1)
      static_cast<SubClass *>(this)->Destroy();
  }
  bool IsUnique() const {
    return mRefCounted.load(std::memory_order_relaxed) == 1;
  }

 protected:
  void Destroy() { delete static_cast<SubClass *>(this); }

 private:
  std::atomic<unsigned> mRefCounted;
};
template <typename T>
class RCReference {
 public:
  RCReference() : mPointer(nullptr) {}
  RCReference(RCReference &&other) : mPointer(other.mPointer) {
    other.mPointer = nullptr;
  }
  template <typename U,
            typename = std::enable_if_t<std::is_base_of<T, U>::value>>
  RCReference(RCReference<U> &&u) : mPointer(u.mPointer) {
    u.mPointer = nullptr;
  }
  RCReference &operator=(RCReference &&other) {
    if (mPointer) mPointer->DropRef();
    mPointer = other.mPointer;
    other.mPointer = nullptr;
    return *this;
  }
  ~RCReference() {
    if (mPointer != nullptr) mPointer->DropRef();
  }
  void reset(T *pointer = nullptr) {
    if (mPointer != nullptr) mPointer->DropRef();
    mPointer = pointer;
  }
  T *release() {
    T *tmp = mPointer;
    mPointer = nullptr;
    return tmp;
  }
  // Not implicity copyable, use the CopyRef() method for an explicit copy of
  // this reference.
  RCReference(const RCReference &) = delete;
  RCReference &operator=(const RCReference &) = delete;

  // RCReference(const RCReference& other) {
  //     if (other.mPointer != nullptr) {
  //         mPointer = other.mPointer;
  //         mPointer->AddRef();
  //     }
  // }
  // RCReference& operator=(const RCReference& other) {
  //     if (other.mPointer != nullptr) {
  //         mPointer = other.mPointer;
  //         mPointer->AddRef();
  //     }
  //     return *this;
  // }

  T &operator*() const {
    assert(mPointer && "null RCReference");
    return *mPointer;
  }

  T *operator->() const {
    assert(mPointer && "null RCReference");
    return mPointer;
  }

  // Return a raw pointer.
  T *get() const { return mPointer; }

  // Make an explicit copy of this RCReference, increasing the refcount by
  // one.
  RCReference CopyRef() const;

  explicit operator bool() const { return mPointer != nullptr; }

  void swap(RCReference &other) { std::swap(mPointer, other.mPointer); }

 private:
  template <typename R>
  friend class RCReference;
  template <typename R>
  friend RCReference<R> FormRef(R *);
  template <typename R>
  friend RCReference<R> TakeRef(R *);
  T *mPointer;
};

template <typename T>
RCReference<T> FormRef(T *pointer) {
  RCReference<T> ref;
  ref.mPointer = pointer;
  pointer->AddRef();
  return ref;
}

template <typename T>
RCReference<T> TakeRef(T *pointer) {
  RCReference<T> ref;
  ref.mPointer = pointer;
  return ref;
}
template <typename T>
RCReference<T> RCReference<T>::CopyRef() const {
  if (!mPointer) return RCReference();
  return FormRef(get());
}
template <typename T>
void swap(RCReference<T> &a, RCReference<T> &b) {
  a.swap(b);
}

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_REF_COUNT_ */
