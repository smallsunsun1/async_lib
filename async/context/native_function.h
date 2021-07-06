#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_NATIVE_FUNCTION_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_NATIVE_FUNCTION_

#include <string_view>

#include "absl/types/span.h"
#include "async/context/host_context.h"
#include "async_value.h"
#include "function.h"

namespace sss {
namespace async {
using NativeCallable = void (*)(AsyncValue* const* arguments, int numArguments, RCReference<AsyncValue>* results, int num_results, HostContext* host);

// 这个类是最常用的简单Function的实现，用于包装一般的回调函数
class SimpleFunction : public Function {
 public:
  SimpleFunction(std::string_view name, NativeCallable callable) : Function(name), mCallable(callable) {}
  void Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const override;
  void AddRef() const override {}
  void DropRef() const override {}

 private:
  NativeCallable mCallable;
};

template <typename T>
class GeneralFunction : public Function {
 public:
  // 这里使用GeneralCallable 来表示lambda表达式的类别
  using GeneralCallable = T;
  GeneralFunction(std::string_view name, GeneralCallable callable) : Function(name), mCallable(std::move(callable)) {}
  void Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const override;
  void AddRef() const override {}
  void DropRef() const override {}

 private:
  GeneralCallable mCallable;
};

template <typename T>
void GeneralFunction<T>::Execute(absl::Span<AsyncValue* const> arguments, absl::Span<RCReference<AsyncValue>> results, HostContext* host) const {
  std::vector<AsyncValue*> unavailableArgs;
  for (auto* av : arguments) {
    if (!av->IsAvailable()) unavailableArgs.push_back(av);
  }
  if (unavailableArgs.empty()) {
    mCallable(arguments.data(), arguments.size(), results.data(), results.size(), host);
    return;
  }
  std::vector<RCReference<AsyncValue>> args;
  args.reserve(arguments.size());
  for (auto* av : arguments) args.push_back(FormRef(av));

  std::vector<RCReference<IndirectAsyncValue>> indirectResults;
  indirectResults.reserve(results.size());
  for (auto& avRef : results) {
    indirectResults.push_back(host->MakeIndirectAsyncValue());
    avRef = indirectResults.back().CopyRef();
  }
  host->RunWhenReady(unavailableArgs, [this, host, args = std::move(args), indirectResults = std::move(indirectResults)]() mutable {
    std::vector<AsyncValue*> argAvs;
    argAvs.reserve(args.size());
    for (const auto& arg : args) argAvs.push_back(arg.get());
    std::vector<RCReference<AsyncValue>> results;
    results.resize(indirectResults.size());
    mCallable(argAvs.data(), argAvs.size(), results.data(), results.size(), host);
    for (int i = 0, e = results.size(); i != e; ++i) {
      assert(results[i]);
      indirectResults[i]->ForwardTo(std::move(results[i]));
    }
  });
}

// 工具类，用于生成最终的Function类别
template <typename GeneralCallable>
Function* NewFunction(std::string_view name, GeneralCallable&& callable) {
  return new GeneralFunction<GeneralCallable>(name, std::forward<GeneralCallable>(callable));
}

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_NATIVE_FUNCTION_ */
