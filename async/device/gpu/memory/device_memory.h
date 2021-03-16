#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_MEMORY_DEVICE_MEMORY_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_MEMORY_DEVICE_MEMORY_

#include <iostream>
#include <type_traits>

#include "async/support/ref_count.h"
#include "async/support/type_traits.h"

namespace ficus {
namespace async {
namespace gpu {

enum class DeviceType { None, CUDA };

std::ostream& operator<<(std::ostream& os, DeviceType dType) {
  std::string s;
  if (dType == DeviceType::None) {
    s = "None";
  } else {
    s = "CUDA";
  }
  os << " DeviceType: " << s;
}

template <typename T>
class DeviceMemory {
 public:
  class PointDeviceTypePair {
   public:
    PointDeviceTypePair() = default;
    PointDeviceTypePair(T* data, DeviceType type) : mpData(data), mType(type) {}
    T* getPointer() const { return mpData; }
    DeviceType getInt() const { return mType; }
    void setPointer(T* data) { mpData = data; }
    void setPointerAndInt(T* data, DeviceType type) {
      mpData = data;
      mType = type;
    }
    bool operator==(const PointDeviceTypePair& other) const { return mpData == other.mpData && mType == other.mType; }
    bool operator!=(const PointDeviceTypePair& other) const { return !operator==(other); }

   private:
    T* mpData = nullptr;
    DeviceType mType;
  };
  template <typename U>
  using implicit_convertible_t = typename std::enable_if_t<std::is_constructible<U*, T*>::value, bool>;
  template <typename U>
  using explicit_convertible_t =
      typename std::enable_if_t<!std::is_constructible<U*, T*>::value && (std::is_constructible<T*, U*>::value || std::is_same<typename std::remove_cv_t<U>, void>::value), bool>::type;
  template <typename U>
  friend class DeviceMemory;
  DeviceMemory() = default;
  DeviceMemory(std::nullptr_t) : DeviceMemory() {}
  DeviceMemory(T* ptr, DeviceType dType) : mPair(ptr, dType) {}
  DeviceMemory(const DeviceMemory&) = default;
  template <typename U, implicit_convertible_t<U> = true>
  DeviceMemory(const DeviceMemory<U>& other) : mPair(other.Pointer(), other.GetDeviceType()) {}
  template <typename U, explicit_convertible_t<U> = false>
  explicit DeviceMemory(const DeviceMemory<U>& other) : mPair(static_cast<T*>(other.Pointer()), other.GetDeviceType()) {}
  template <typename U>
  DeviceMemory& operator=(const DeviceMemory<U>& other) {
    mPair.setPointerAndInt(static_cast<T*>(other.Pointer()), other.GetDeviceType());
    return *this;
  }
  DeviceMemory& operator=(const DeviceMemory&) = default;
  DeviceMemory& operator=(std::nullptr_t) { return *this = DeviceMemory(); }

  DeviceType GetDeviceType() { return mPair.getInt(); }
  T* data(DeviceType dType) const {
    assert(mPair.getInt() == dType);
    return Pointer();
  }
  T* data() const { return Pointer(); }

  DeviceMemory operator+(std::ptrdiff_t offset) const { return DeviceMemory(Pointer() + offset, GetDeviceType()); }
  DeviceMemory operator-(std::ptrdiff_t offset) const { return DeviceMemory(Pointer() - offset, GetDeviceType()); }
  DeviceMemory& operator+=(std::ptrdiff_t offset) {
    mPair.setPointer(Pointer() + offset);
    return *this;
  }
  DeviceMemory& operator-=(std::ptrdiff_t offset) {
    mPair.setPointer(Pointer() - offset);
    return *this;
  }
  DeviceMemory& operator++() { return *this += 1; }
  DeviceMemory& operator--() { return *this -= 1; }
  DeviceMemory operator++(int) {
    DeviceMemory result = *this;
    ++*this;
    return result;
  }
  DeviceMemory operator--(int) {
    DeviceMemory result = *this;
    --*this;
    return result;
  }

  explicit operator bool() const { return *this != nullptr; }
  bool operator!() const { return *this == nullptr; }

  bool operator==(std::nullptr_t) const { return Pointer() == nullptr; }
  bool operator!=(std::nullptr_t) const { return Pointer() != nullptr; }

  template <typename U>
  bool operator==(const DeviceMemory<U>& other) const {
    return mPair == other.mPair;
  }
  template <typename U>
  bool operator!=(const DeviceMemory<U>& other) const {
    return mPair != other.mPair;
  }

  template <typename U>
  bool operator<(const DeviceMemory<U>& other) const {
    return Pointer() < other.Pointer();
  }
  template <typename U>
  bool operator<=(const DeviceMemory<U>& other) const {
    return Pointer() <= other.Pointer();
  }

  template <typename U>
  bool operator>(const DeviceMemory<U>& other) const {
    return Pointer() > other.Pointer();
  }
  template <typename U>
  bool operator>=(const DeviceMemory<U>& other) const {
    return Pointer() >= other.Pointer();
  }

 private:
  T* Pointer() { return mPair.getPointer(); }
  typename std::conditional<PointerLikeTypeTraits<T*>::NumLowBitsAvailable >= 2, PointerIntPair<T*, 2, DeviceType>, PointDeviceTypePair>::type
      mPair;  //通常指针align大小为3bit，这里小于2bit可以压缩到一个PointIntPair
  friend std::ostream& operator<<(std::ostream& os, const DeviceMemory& Pointer) { return os << Pointer.data() << "\n"; }
};

}  // namespace gpu
}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_MEMORY_DEVICE_MEMORY_ \
        */
