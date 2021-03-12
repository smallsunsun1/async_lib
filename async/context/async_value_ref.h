#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_ASYNC_VALUE_REF_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_ASYNC_VALUE_REF_

#include "../support/ref_count.h"
#include "async_value.h"
#include "diagnostic.h"
#include "location.h"
#include "third_party/abseil-cpp/absl/status/statusor.h"

namespace ficus {
namespace async {
class ExecutionContext;
template <typename T>
class AsyncValueRef {
 public:
  AsyncValueRef() = default;
  // Support implicit conversion from AsyncValueRef<Derived> to
  // AsyncValueRef<Base>.
  template <typename DerivedT,
            std::enable_if_t<std::is_base_of<T, DerivedT>::value, int> = 0>
  AsyncValueRef(AsyncValueRef<DerivedT>&& u) : mValue(u.ReleaseRCRef()) {}

  explicit AsyncValueRef(RCReference<AsyncValue> value)
      : mValue(std::move(value)) {}

  // Support implicit conversion from RCReference<AsyncValue>.
  AsyncValueRef(RCReference<ErrorAsyncValue> value)
      : mValue(std::move(value)) {}

  AsyncValueRef& operator=(RCReference<ErrorAsyncValue> new_value) {
    mValue = std::move(new_value);
    return *this;
  }

  // Allow implicit conversion to type-erased RCReference<AsyncValue>
  operator RCReference<AsyncValue>() && { return std::move(mValue); }

  // Return true if the AsyncValue is resolved to a concrete value or error.
  bool IsAvailable() const { return mValue->IsAvailable(); }
  bool IsUnavailable() const { return mValue->IsUnavailable(); }

  // Return true if the AsyncValue contains a concrete value.
  bool IsConcrete() const { return mValue->IsConcrete(); }

  // Return the stored value. The AsyncValueRef must be available.
  T& get() const { return mValue->get<T>(); }

  // Return the stored value as a subclass type. The AsyncValueRef must be
  // available.
  template <typename SubclassT,
            typename = std::enable_if_t<std::is_base_of<T, SubclassT>::value>>
  SubclassT& get() const {
    return mValue->get<SubclassT>();
  }

  T* operator->() const { return &get(); }

  T& operator*() const { return get(); }

  // Make the AsyncValueRef available.
  void SetStateConcrete() const { mValue->SetStateConcrete(); }

  // Set the stored value. The AsyncValueRef must be unavailable. After this
  // returns, the AsyncValueRef will be available.
  template <typename... Args>
  void emplace(Args&&... args) const {
    mValue->emplace<T>(std::forward<Args>(args)...);
  }

  void emplace(absl::StatusOr<T> v) const {
    if (v) {
      emplace(std::move(*v));
    } else {
      SetError(v.status());
    }
  }

  // If the AsyncValueRef is available, run the waiter immediately. Otherwise,
  // run the waiter when the AsyncValueRef becomes available.
  template <typename WaiterT>
  void AndThen(WaiterT&& waiter) const {
    mValue->AndThen(std::move(waiter));
  }

  // Return true if this AsyncValueRef represents an error.
  bool IsError() const { return mValue->IsError(); }

  // Returns the underlying error. IsError() must be true.
  const DecodedDiagnostic& GetError() const { return mValue->GetError(); }

  // Returns the underlying error, or nullptr if there is none.
  const DecodedDiagnostic* GetErrorIfPresent() const {
    return mValue->GetErrorIfPresent();
  }

  void SetError(absl::string_view message) const {
    return SetError(DecodedDiagnostic{message});
  }
  void SetError(DecodedDiagnostic diag) const {
    mValue->SetError(std::move(diag));
  }

  void SetError(const absl::Status& error) const {
    mValue->SetError(DecodedDiagnostic(error));
  }

  explicit operator bool() const { return mValue.get() != nullptr; }

  // Return a raw pointer to the AsyncValue.
  AsyncValue* GetAsyncValue() const { return mValue.get(); }

  // Return true if this is the only ref to the AsyncValue.
  // This function requires the internal AsyncValue to be set (mValue !=
  // nullptr).
  bool IsUnique() const { return mValue->IsUnique(); }

  // Make an explicit copy of this AsyncValueRef, increasing mValue's refcount
  // by one.
  AsyncValueRef<T> CopyRef() const { return AsyncValueRef(CopyRCRef()); }

  // Make a copy of mValue, increasing mValue's refcount by one.
  RCReference<AsyncValue> CopyRCRef() const { return mValue.CopyRef(); }

  // Release ownership of one reference on the AsyncValue and return a raw
  // pointer to it.
  AsyncValue* release() { return mValue.release(); }

  void reset() { mValue.reset(); }

  // Transfer ownership of one reference on the AsyncValue to the returned
  // RCReference<AsyncValue>.
  RCReference<AsyncValue> ReleaseRCRef() { return std::move(mValue); }

 private:
  RCReference<AsyncValue> mValue;
};

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_ASYNC_VALUE_REF_ */
