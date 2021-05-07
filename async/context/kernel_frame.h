#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_KERNEL_FRAME_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_KERNEL_FRAME_

#include "async/support/string_util.h"
#include "host_context.h"
#include "absl/container/inlined_vector.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace sss {
namespace async {

// 这个类不负责对其中的AsyncValue*进行析构，需要手动析构
class AsyncKernelFrame {
 public:
  explicit AsyncKernelFrame(HostContext* ctx) : mpContext(ctx) {}
  HostContext* GetHostContext() const { return mpContext; }
  int GetNumArguments() const { return mNumArguments; }
  template <typename T>
  T& GetArgAt(int index) const {
    return GetArgAt(index)->get<T>();
  }
  AsyncValue* GetArgAt(int index) const {
    assert(index < GetNumArguments() && "Get Arg Index Must Smaller Than Num Arguments!");
    return mAsyncValues[index];
  }
  absl::Span<AsyncValue* const> GetArguments() const { return GetAsyncValues(0, mNumArguments); }
  int GetNumResults() const { return mNumResults; }
  template <typename T, typename... Args>
  void EmplaceResultAt(int index, Args&&... args) {
    SetResultAt(index, mpContext->MakeAvailableAsyncValueRef<T>(std::forward<Args>(args)...));
  }
  template <typename T, typename... Args>
  void EmplaceResult(Args&&... args) {
    EmplaceResultAt<T>(0, std::forward<Args>(args)...);
  }
  void SetResultAt(int index, RCReference<AsyncValue> value) {
    assert(index < mNumResults && "Invalid Result Index");
    AsyncValue*& result = mAsyncValues[mNumArguments + index];
    assert(!result && "result must be nullptr!");
    result = value.release();
  }
  template <typename T>
  void SetResultAt(int index, AsyncValueRef<T> value) {
    SetResultAt(index, value.ReleaseRCRef());
  }
  template <typename T>
  AsyncValueRef<T> AllocateResultAt(int index) {
    auto result = mpContext->MakeUnconstructedAsyncValueRef<T>();
    SetResultAt(index, result.CopyRef());
    return result;
  }
  template <typename T>
  AsyncValueRef<T> AllocateResult() {
    return AllocateResultAt<T>(0);
  }
  RCReference<AsyncValue> AllocateIndirectResultAt(int index) {
    auto result = mpContext->MakeIndirectAsyncValue();
    SetResultAt(index, result.CopyRef());
    return result;
  }
  absl::Span<AsyncValue* const> GetResults() const { return GetAsyncValues(mNumArguments, mNumResults); }
  absl::Span<AsyncValue*> GetResults() { return GetMutableAsyncValues(mNumArguments, mNumResults); }
  template <typename... Args>
  RCReference<AsyncValue> EmitError(Args&&... args) {
    return mpContext->MakeErrorAsyncValueRef(StrCat(std::forward<Args>(args)...));
  }
  // 对错误信息进行广播
  void PropogateError(absl::string_view errorMsg) {
    bool setError = false;
    RCReference<ErrorAsyncValue> errorValue = mpContext->MakeErrorAsyncValueRef(errorMsg);
    for (auto& result : GetResults()) {
      if (!result) {
        result = errorValue.CopyRef().release();
        setError = true;
      } else if (result->IsUnavailable()) {
        result->SetError(errorValue->GetError());
        setError = true;
      }
    }
    assert(setError && "PropogateError Must Set An Error Value");
  }

 protected:
  absl::Span<AsyncValue* const> GetAsyncValues(size_t from, size_t length) const {
    assert((from + length) <= (mNumArguments + mNumResults) && "from + length Must Less Than Total Asyncvalue Size");
    if (length == 0) return {};
    return absl::MakeConstSpan(&mAsyncValues[from], length);
  }
  absl::Span<AsyncValue*> GetMutableAsyncValues(size_t from, size_t length) {
    assert((from + length) <= (mNumArguments + mNumResults) && "from + length Must Less Than Total Asyncvalue Size");
    if (length == 0) return {};
    return absl::MakeSpan(&mAsyncValues[from], length);
  }

 protected:
  absl::InlinedVector<AsyncValue*, 8> mAsyncValues;
  int mNumArguments = 0;
  int mNumResults = -1;
  HostContext* mpContext;
};

class CommonAsyncKernelFrame : public AsyncKernelFrame {
 public:
  using AsyncKernelFrame::AsyncKernelFrame;
  AsyncValue* GetResultAt(int index) const { return GetResults()[index]; }
  void AddArg(AsyncValue* argument) {
    assert(mNumResults == -1 && "Must Call AddArg Before Calling SetNumResults");
    mAsyncValues.push_back(argument);
    mNumArguments++;
  }
  void SetNumResults(int size) {
    assert(mNumArguments == mAsyncValues.size() && "NumArguments Must Equal to AsyncValues size");
    assert(mNumResults == -1 && "SetNumResults Only Can Be Called Once");
    mAsyncValues.resize(mNumArguments + size);
    mNumResults = size;
  }
  void Reset() {
    mAsyncValues.clear();
    mNumResults = -1;
    mNumArguments = 0;
  }
};

// 通常用于异步function参数，用于保证输入和输出RAII
class RAIIKernelFrame : public AsyncKernelFrame {
 public:
  RAIIKernelFrame() = delete;
  RAIIKernelFrame(const AsyncKernelFrame& frame) : AsyncKernelFrame(frame) { AddRefAll(); }
  RAIIKernelFrame(const RAIIKernelFrame& that) : AsyncKernelFrame(that) { AddRefAll(); }
  ~RAIIKernelFrame() {
    if (!mAsyncValues.empty()) {
      DropRefAll();
    }
  }

 private:
  void AddRefAll() const {
    for (auto* v : GetAsyncValues(0, mNumArguments + mNumResults)) {
      v->AddRef();
    }
  }
  void DropRefAll() const {
    for (auto* v : GetAsyncValues(0, mNumArguments + mNumResults)) {
      v->DropRef();
    }
  }
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_KERNEL_FRAME_ */
