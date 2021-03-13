#ifndef ASYNC_SUPPORT_UNIQUE_FUNCTION_
#define ASYNC_SUPPORT_UNIQUE_FUNCTION_

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include "alloc.h"
#include "extra_structure.h"

namespace ficus {
namespace async {

template <typename FunctionT>
class unique_function;
namespace detail {
template <typename T>
using enable_if_trivial = std::enable_if_t<std::is_trivially_move_constructible<T>::value && std::is_trivially_destructible<T>::value>;
template <typename callableT, typename thisT>
using enable_unless_same_type = std::enable_if_t<!std::is_same<std::decay_t<callableT>, thisT>::value>;
template <typename callableT, typename Ret, typename... Params>
using enable_if_callable = std::enable_if_t<std::is_void<Ret>::value || std::is_convertible<decltype(std::declval<callableT>()(std::declval<Params>()...)), Ret>::value>;
template <typename ReturnT, typename... ParamsTs>
class UniqueFunctionBase {
 protected:
  static constexpr size_t kInlineStorageSize = sizeof(void*) * 3;
  template <typename T, class = void>
  struct IsSizeLessThanThresholdT : std::false_type {};
  template <typename T>
  struct IsSizeLessThanThresholdT<T, std::enable_if_t<sizeof(T) <= 2 * sizeof(void*)>> : std::true_type {};
  template <typename T>
  using adjusted_param_t =
      typename std::conditional<!std::is_reference<T>::value && std::is_trivially_copy_constructible<T>::value && std::is_trivially_move_constructible<T>::value && IsSizeLessThanThresholdT<T>::value,
                                T, T&>::type;
  using call_ptr_t = ReturnT (*)(void* callableAddr, adjusted_param_t<ParamsTs>... Params);
  using move_ptr_t = void (*)(void* lhsCallableAddr, void* rhsCallableAddr);
  using destroy_ptr_t = void (*)(void* callableAddr);
  struct alignas(8) TrivialCallback {
    call_ptr_t callPtr;
  };
  struct alignas(8) NonTrivialCallbacks {
    call_ptr_t callPtr;
    move_ptr_t movePtr;
    destroy_ptr_t destroyPtr;
  };
  using call_back_pointer_union_t = PointerUnion<TrivialCallback*, NonTrivialCallbacks*>;
  union StorageUnionT {
    struct OutOfLineStorageT {
      void* storagePtr;
      size_t size;
      size_t alignment;
    } outOfLineStorage;
    static_assert(sizeof(OutOfLineStorageT) <= kInlineStorageSize, "should always use all of the out-of-line storage for inline storage!");
    mutable typename std::aligned_storage<kInlineStorageSize, alignof(void*)>::type inlineStorage;
  } storageUnion;
  PointerIntPair<call_back_pointer_union_t, 1, bool> callbackAndInlineFlag;
  bool isInlineStorage() const { return callbackAndInlineFlag.getInt(); }
  bool isTrivialCallBack() const { return callbackAndInlineFlag.getPointer().template is<TrivialCallback*>(); }
  call_ptr_t getTrivialCallBack() const { return callbackAndInlineFlag.getPointer().template get<TrivialCallback*>()->callPtr; }
  NonTrivialCallbacks* getNonTrivialCallbacks() const { return callbackAndInlineFlag.getPointer().template get<NonTrivialCallbacks*>(); }
  call_ptr_t getCallPtr() const { return isTrivialCallBack() ? getTrivialCallBack() : getNonTrivialCallbacks()->callPtr; }
  void* getCalleePtr() const { return isInlineStorage() ? getInlineStorage() : getOutOfLineStorage(); }
  void* getInlineStorage() const { return &storageUnion.inlineStorage; }
  void* getOutOfLineStorage() const { return storageUnion.outOfLineStorage.storagePtr; }
  size_t getOutOfLineStorageSize() const { return storageUnion.outOfLineStorage.size; }
  size_t getOutOfLineStorageAlignment() const { return storageUnion.outOfLineStorage.alignment; }
  void setOutOfLineStorage(void* ptr, size_t size, size_t alignment) { storageUnion.outOfLineStorage = {ptr, size, alignment}; }
  template <typename CalledAsT>
  static ReturnT CallImpl(void* callableAddr, adjusted_param_t<ParamsTs>... params) {
    auto& func = *reinterpret_cast<CalledAsT*>(callableAddr);
    return func(std::forward<ParamsTs>(params)...);
  }
  template <typename CallableT>
  static void MoveImpl(void* lhsCallableAddr, void* rhsCallableAddr) noexcept {
    new (lhsCallableAddr) CallableT(std::move(*reinterpret_cast<CallableT*>(rhsCallableAddr)));
  }
  template <typename CallableT>
  static void DestroyImpl(void* callableAddr) noexcept {
    reinterpret_cast<CallableT*>(callableAddr)->~CallableT();
  }
  template <typename CallableT, typename CalledAs, typename Enable = void>
  struct CallbacksHolder {
    static NonTrivialCallbacks callbacks;
  };
  template <typename CallableT, typename CalledAs>
  struct CallbacksHolder<CallableT, CalledAs, enable_if_trivial<CallableT>> {
    static TrivialCallback callbacks;
  };
  template <typename T>
  struct CalledAs {};
  template <typename CallableT, typename CalledAsT>
  UniqueFunctionBase(CallableT callable, CalledAs<CalledAsT>) {
    bool isInlineStorageV = true;
    void* callableAddr = getInlineStorage();
    if (sizeof(CallableT) > kInlineStorageSize || alignof(CallableT) > alignof(decltype(storageUnion.inlineStorage))) {
      isInlineStorageV = false;
      auto size = sizeof(CallableT);
      auto alignemnt = alignof(CallableT);
      callableAddr = AlignedAlloc(alignemnt, size);
      setOutOfLineStorage(callableAddr, size, alignemnt);
    }
    new (callableAddr) CallableT(std::move(callable));
    callbackAndInlineFlag.setPointerAndInt(&CallbacksHolder<CallableT, CalledAsT>::callbacks, isInlineStorageV);
  }
  ~UniqueFunctionBase() {
    if (!callbackAndInlineFlag.getPointer()) return;
    bool isInlineStorageV = isInlineStorage();
    if (!isTrivialCallBack()) {
      getNonTrivialCallbacks()->destroyPtr(isInlineStorageV ? getInlineStorage() : getOutOfLineStorage());
    }
    if (!isInlineStorage()) {
      free(getOutOfLineStorage());
    }
  }
  UniqueFunctionBase(UniqueFunctionBase&& rhs) noexcept {
    callbackAndInlineFlag = rhs.callbackAndInlineFlag;
    if (!rhs) return;
    if (!isInlineStorage()) {
      storageUnion.outOfLineStorage = rhs.storageUnion.outOfLineStorage;
    } else if (isTrivialCallBack()) {
      memcpy(getInlineStorage(), rhs.getInlineStorage(), kInlineStorageSize);
    } else {
      getNonTrivialCallbacks()->movePtr(getInlineStorage(), rhs.getInlineStorage());
    }
    rhs.callbackAndInlineFlag = {};
  }
  UniqueFunctionBase& operator=(UniqueFunctionBase&& rhs) noexcept {
    if (this == &rhs) return *this;
    this->~UniqueFunctionBase();
    new (this) UniqueFunctionBase(std::move(rhs));
    return *this;
  }
  UniqueFunctionBase() = default;

 public:
  explicit operator bool() const { return (bool)callbackAndInlineFlag.getPointer(); }
};
template <typename R, typename... P>
template <typename CallableT, typename CalledAsT, typename Enable>
typename UniqueFunctionBase<R, P...>::NonTrivialCallbacks UniqueFunctionBase<R, P...>::CallbacksHolder<CallableT, CalledAsT, Enable>::callbacks = {&CallImpl<CalledAsT>, &MoveImpl<CallableT>,
                                                                                                                                                   &DestroyImpl<CallableT>};

template <typename R, typename... P>
template <typename CallableT, typename CalledAsT>
typename UniqueFunctionBase<R, P...>::TrivialCallback UniqueFunctionBase<R, P...>::CallbacksHolder<CallableT, CalledAsT, enable_if_trivial<CallableT>>::callbacks{&CallImpl<CalledAsT>};

}  // namespace detail

template <typename R, typename... Ts>
class unique_function<R(Ts...)> : public detail::UniqueFunctionBase<R, Ts...> {
  using Base = detail::UniqueFunctionBase<R, Ts...>;

 public:
  unique_function() = default;
  unique_function(std::nullptr_t) {}
  unique_function(unique_function&&) = default;
  unique_function(const unique_function&) = delete;
  unique_function& operator=(unique_function&&) = default;
  unique_function& operator=(const unique_function&) = delete;
  template <typename CallableT>
  unique_function(CallableT callable, detail::enable_unless_same_type<CallableT, unique_function>* = nullptr, detail::enable_if_callable<CallableT, R, Ts...>* = nullptr)
      : Base(std::forward<CallableT>(callable), typename Base::template CalledAs<CallableT>{}) {}
  R operator()(Ts... params) { return this->getCallPtr()(this->getCalleePtr(), params...); }
};
template <typename R, typename... Ts>
class unique_function<R(Ts...) const> : public detail::UniqueFunctionBase<R, Ts...> {
  using Base = detail::UniqueFunctionBase<R, Ts...>;

 public:
  unique_function() = default;
  unique_function(std::nullptr_t) {}
  unique_function(unique_function&&) = default;
  unique_function(const unique_function&) = delete;
  unique_function& operator=(unique_function&&) = default;
  unique_function& operator=(const unique_function&) = delete;
  template <typename CallableT>
  unique_function(CallableT callable, detail::enable_unless_same_type<CallableT, unique_function>* = nullptr, detail::enable_if_callable<const CallableT, R, Ts...>* = nullptr)
      : Base(std::forward<CallableT>(callable), typename Base::template CalledAs<const CallableT>{}) {}
  R operator()(Ts... params) const { return this->getCallPtr()(this->getCalleePtr(), params...); }
};

}  // namespace async
}  // namespace ficus

#endif /* ASYNC_SUPPORT_UNIQUE_FUNCTION_ */
