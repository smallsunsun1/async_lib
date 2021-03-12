#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_STREAM_STREAM_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_STREAM_STREAM_

#include "../memory/device_memory.h"
#include "cuda.h"

namespace ficus {
namespace async {
namespace gpu {
namespace stream {

class Device {
 public:
  Device() = default;
  Device(int deviceId, DeviceType dType) : mDeviceId(deviceId), mType(dType) {}
  int Id(DeviceType dType) const {
    assert(mType == dType);
    return mDeviceId;
  }
  DeviceType getDeviceType() const { return mType; }
  bool operator==(const Device& other) const {
    return mDeviceId == other.mDeviceId && mType == other.mType;
  }
  bool operator!=(const Device& other) const { return !(*this == other); }

 private:
  static_assert(std::is_same<CUdevice, int>::value, "");
  int mDeviceId = 0;                    // 默认使用0号设备
  DeviceType mType = DeviceType::None;  // 未知设备
  friend std::ostream& operator<<(std::ostream& os, Device device) {
    os << "deviceId: " << device.mDeviceId << " deviceType: " << device;
  }
};

template <typename CudaT, typename HipT>
class Resource {
 public:
  Resource() = default;
  Resource(std::nullptr_t, DeviceType dType) : mPair(nullptr, dType) {}
  explicit Resource(void* ptr) : mPair(ptr, DeviceType::None) {}
  Resource(CudaT ptr) : mPair(ptr, DeviceType::CUDA) {}
  Resource(HipT ptr) : mPair(ptr, DeviceType::None) {}
  DeviceType getDeviceType() const { return mPair.getInt(); }
  operator bool() const { return *this != nullptr; }
  operator HipT() const { return static_cast<HipT>(mPair.getPointer()); }
  operator CudaT() const {
    // 这里需要添加T类型所对应的DeviceType的assert断言
    return static_cast<CudaT>(mPair.getPointer());
  }
  bool operator==(std::nullptr_t) const {
    return mPair.getPointer() == nullptr;
  }
  bool operator!=(std::nullptr_t) const {
    return mPair.getPointer() != nullptr;
  }
  bool operator==(const Resource& other) const { return mPair == other.mPair; }
  bool operator!=(const Resource& other) const { return mPair != other.mPair; }
  size_t hash() const noexcept {
    return std::hash<void*>()(mPair.getOpaqueValue());
  }

 private:
  llvm::PointerIntPair<void*, 2, DeviceType> mPair;
  friend std::ostream& operator<<(std::ostream& os, const Resource& resource) {
    os << resource.mPair.getPointer() << ": " << resource.getDeviceType();
  }
};

// 当前仅有Cuda设备支持
using Context = Resource<CUstream, CUstream>;
using Module = Resource<CUmodule, CUmodule>;
using Stream = Resource<CUstream, CUstream>;
using Event = Resource<CUevent, CUevent>;
using Function = Resource<CUfunction, CUfunction>;

}  // namespace stream
}  // namespace gpu
}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_DEVICE_GPU_STREAM_STREAM_ */
