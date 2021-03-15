#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_CONTEXT_PTR_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_CONTEXT_PTR_

#include <cassert>
#include <cstdint>

namespace ficus {
namespace async {
class HostContext;
class HostContextPtr {
 public:
  // Implicitly convert HostContext* to HostContextPtr.
  HostContextPtr(HostContext* host);  // NOLINT
  HostContextPtr() = default;

  HostContext* operator->() const { return get(); }

  HostContext& operator*() const { return *get(); }

  HostContext* get() const;

  explicit operator bool() const { return mIndex == kDummyIndex; }

 private:
  friend class HostContext;

  explicit HostContextPtr(int index) : mIndex{static_cast<uint8_t>(index)} { assert(index < kDummyIndex); }
  uint8_t index() const { return mIndex; }

  static constexpr uint8_t kDummyIndex = 255;
  const uint8_t mIndex = kDummyIndex;
};

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_HOST_CONTEXT_PTR_ */
