#ifndef ASYNC_SUPPORT_EXTRA_STRUCTURE_
#define ASYNC_SUPPORT_EXTRA_STRUCTURE_

#include <algorithm>

#include "type_traits.h"

namespace sss {
namespace async {

template <typename T>
struct PointerUnionTypeSelectorReturn {
  using Return = T;
};
template <typename T1, typename T2, typename RetEq, typename RetNe>
struct PointerUnionTypeSelector {
  using Return = typename PointerUnionTypeSelectorReturn<RetNe>::Return;
};
template <typename T, typename RetEq, typename RetNe>
struct PointerUnionTypeSelector<T, T, RetEq, RetNe> {
  using Return = typename PointerUnionTypeSelectorReturn<RetEq>::Return;
};
template <typename T1, typename T2, typename RetEq, typename RetNe>
struct PointerUnionTypeSelectorReturn<PointerUnionTypeSelector<T1, T2, RetEq, RetNe>> {
  using Return = typename PointerUnionTypeSelector<T1, T2, RetEq, RetNe>::Return;
};
namespace detail {
constexpr int bitsRequired(unsigned n) { return n > 1 ? 1 + bitsRequired((n + 1) / 2) : 0; }
template <typename... Ts>
constexpr int lowBitsAvailable() {
  return std::min<int>({PointerLikeTypeTraits<Ts>::NumLowBitsAvailable...});
}
template <typename T, typename... Us>
struct TypeIndex;
template <typename T, typename... Us>
struct TypeIndex<T, T, Us...> {
  static constexpr int Index = 0;
};
template <typename T, typename U, typename... Us>
struct TypeIndex<T, U, Us...> {
  static constexpr int Index = 1 + TypeIndex<T, Us...>::Index;
};
template <typename T>
struct TypeIndex<T> {
  static constexpr int Index = 0;
};
template <typename T, typename... Us>
struct GetFirstType {
  using type = T;
};
template <typename... Pts>
class PointerUnionUIntTraits {
 public:
  static inline void *getAsVoidPointer(void *P) { return P; }
  static inline void *getFromVoidPointer(void *P) { return P; }
  static constexpr int NumLowBitsAvailable = lowBitsAvailable<Pts...>();
};
template <typename Derived, typename ValTy, int I, typename... Types>
class PointerUnionMembers;
template <typename Derived, typename ValTy, int I>
class PointerUnionMembers<Derived, ValTy, I> {
 protected:
  ValTy Val;
  PointerUnionMembers() = default;
  PointerUnionMembers(ValTy Val) : Val(Val) {}
  friend struct PointerLikeTypeTraits<Derived>;
};
template <typename Derived, typename ValTy, int I, typename Type, typename... Types>
class PointerUnionMembers<Derived, ValTy, I, Type, Types...> : public PointerUnionMembers<Derived, ValTy, I + 1, Types...> {
  using Base = PointerUnionMembers<Derived, ValTy, I + 1, Types...>;

 public:
  using Base::Base;
  PointerUnionMembers() = default;
  PointerUnionMembers(Type v) : Base(ValTy(const_cast<void *>(PointerLikeTypeTraits<Type>::getAsVoidPointer(v)), I)) {}
  using Base::operator=;
  Derived &operator=(Type v) {
    this->val = ValTy(const_cast<void *>(PointerLikeTypeTraits<Type>::getAsVoidPointer(v)), I);
    return static_cast<Derived &>(*this);
  }
};
}  // namespace detail

template <typename... Pts>
class PointerUnion : public detail::PointerUnionMembers<PointerUnion<Pts...>, PointerIntPair<void *, detail::bitsRequired(sizeof...(Pts)), int, detail::PointerUnionUIntTraits<Pts...>>, 0, Pts...> {
  using First = typename detail::GetFirstType<Pts...>::type;
  using Base = typename PointerUnion::PointerUnionMembers;

 public:
  PointerUnion() = default;
  PointerUnion(std::nullptr_t) : PointerUnion(){};
  using Base::Base;
  bool isNull() const { return !this->Val.getPointer(); }
  explicit operator bool() const { return !isNull(); }
  template <typename T>
  bool is() const {
    constexpr int Index = detail::TypeIndex<T, Pts...>::Index;
    static_assert(Index < sizeof...(Pts), "PointerUnion::is<T> given type not in the union");
    return this->Val.getInt() == Index;
  }
  template <typename T>
  T get() const {
    assert(is<T>() && "Invalid accessor called");
    return PointerLikeTypeTraits<T>::getFromVoidPointer(this->Val.getPointer());
  }
  template <typename T>
  T dyn_cast() const {
    if (is<T>()) return get<T>();
    return T();
  }
  First *getAddrOfPtr1() {
    assert(is<First>() && "Val is not the first pointer");
    assert(PointerLikeTypeTraits<First>::getAsVoidPointer(get<First>()) == this->Val.getPointer() && "Can't get the address because PointerLikeTypeTraits changes the ptr");
    return const_cast<First *>(reinterpret_cast<const First *>(this->Val.getAddrOfPointer()));
  }

  const PointerUnion &operator=(std::nullptr_t) {
    this->Val.initWithPointer(nullptr);
    return *this;
  }
  using Base::operator=;
  void *getOpaqueValue() const { return this->Val.getOpaqueValue(); }
  static inline PointerUnion getFromOpaqueValue(void *VP) {
    PointerUnion V;
    V.Val = decltype(V.Val)::getFromOpaqueValue(VP);
    return V;
  }
};

template <typename... PTs>
bool operator==(PointerUnion<PTs...> lhs, PointerUnion<PTs...> rhs) {
  return lhs.getOpaqueValue() == rhs.getOpaqueValue();
}

template <typename... PTs>
bool operator!=(PointerUnion<PTs...> lhs, PointerUnion<PTs...> rhs) {
  return lhs.getOpaqueValue() != rhs.getOpaqueValue();
}

template <typename... PTs>
bool operator<(PointerUnion<PTs...> lhs, PointerUnion<PTs...> rhs) {
  return lhs.getOpaqueValue() < rhs.getOpaqueValue();
}

template <typename... PTs>
struct PointerLikeTypeTraits<PointerUnion<PTs...>> {
  static inline void *getAsVoidPointer(const PointerUnion<PTs...> &P) { return P.getOpaqueValue(); }

  static inline PointerUnion<PTs...> getFromVoidPointer(void *P) { return PointerUnion<PTs...>::getFromOpaqueValue(P); }

  // The number of bits available are the min of the pointer types minus the
  // bits needed for the discriminator.
  static constexpr int NumLowBitsAvailable = PointerLikeTypeTraits<decltype(PointerUnion<PTs...>::Val)>::NumLowBitsAvailable;
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_SUPPORT_EXTRA_STRUCTURE_ */
