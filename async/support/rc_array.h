#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_RC_ARRAY_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_RC_ARRAY_

#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ref_count.h"

namespace sss {
namespace async {
template <typename T>
class RCArray {
 public:
  RCArray() {}
  explicit RCArray(absl::Span<const T*> values) {
    mValues.reserve(values.size());
    for (auto* v : values) {
      v->AddRef();
      mValues.push_back(v);
    }
  }
  explicit RCArray(absl::Span<const RCReference<T>> references) {
    mValues.reserve(references.size());
    for (auto& ref : references) {
      auto* v = ref.get();
      v->AddRef();
      mValues.push_back(v);
    }
  }
  RCArray(RCArray&& other) : mValues(std::move(other.mValues)) {}
  RCArray& operator=(RCArray&& other) {
    for (auto* v : mValues) {
      v->DropRef();
    }
    mValues = std::move(other.mValues);
    return *this;
  }
  ~RCArray() {
    for (auto* v : mValues) {
      v->DropRef();
    }
  }
  T* operator[](size_t i) const {
    assert(i < mValues.size());
    return mValues[i];
  }
  RCArray CopyRef() const { return RCArray(values()); }
  absl::Span<const T*> values() const { return mValues; }
  size_t size() const { return mValues.size(); }
  // Not copyable
  RCArray(const RCArray&) = delete;
  RCArray& operator=(const RCArray&) = delete;

 private:
  absl::InlinedVector<T*, 4> mValues;
};
}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_RC_ARRAY_ */
