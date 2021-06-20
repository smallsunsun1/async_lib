#ifndef ASYNC_SUPPORT_TYPE_TRAITS_
#define ASYNC_SUPPORT_TYPE_TRAITS_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace sss {
namespace async {

template <typename T>
struct remove_cv_ref {
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template <typename T>
using remove_cv_ref_t = typename remove_cv_ref<T>::type;

template <typename T>
struct TypeTag {};

template <typename T>
struct PointerLikeTypeTraits;
namespace detail {
template <size_t N>
struct ConstantLog2 : public std::integral_constant<size_t, ConstantLog2<N / 2>::value + 1> {};
template <>
struct ConstantLog2<1> : public std::integral_constant<size_t, 0> {};
template <typename T, typename U = void>
struct HasPointerLikeTypeTraits {
  static constexpr bool value = false;
};
template <typename T>
struct HasPointerLikeTypeTraits<T, decltype((sizeof(PointerLikeTypeTraits<T>) + sizeof(T)), void())> {
  static constexpr bool value = true;
};
template <typename T>
struct IsPointerLike {
  static constexpr bool value = HasPointerLikeTypeTraits<T>::value;
};
template <typename T>
struct IsPointerLike<T *> {
  static constexpr bool value = true;
};
}  // namespace detail
template <typename T>
struct PointerLikeTypeTraits<T *> {
  static inline void *getAsVoidPointer(T *P) { return P; }
  static inline T *getFromVoidPointer(void *P) { return static_cast<T *>(P); }
  static constexpr int NumLowBitsAvailable = detail::ConstantLog2<alignof(T)>::value;
};

template <>
struct PointerLikeTypeTraits<void *> {
  static inline void *getAsVoidPointer(void *P) { return P; }
  static inline void *getFromVoidPointer(void *P) { return P; }
  static constexpr int NumLowBitsAvailable = 2;
};
template <typename T>
struct PointerLikeTypeTraits<const T> {
  typedef PointerLikeTypeTraits<T> NonConst;

  static inline const void *getAsVoidPointer(const T P) { return NonConst::getAsVoidPointer(P); }
  static inline const T getFromVoidPointer(const void *P) { return NonConst::getFromVoidPointer(const_cast<void *>(P)); }
  static constexpr int NumLowBitsAvailable = NonConst::NumLowBitsAvailable;
};
template <typename T>
struct PointerLikeTypeTraits<const T *> {
  typedef PointerLikeTypeTraits<T *> NonConst;

  static inline const void *getAsVoidPointer(const T *P) { return NonConst::getAsVoidPointer(const_cast<T *>(P)); }
  static inline const T *getFromVoidPointer(const void *P) { return NonConst::getFromVoidPointer(const_cast<void *>(P)); }
  static constexpr int NumLowBitsAvailable = NonConst::NumLowBitsAvailable;
};

template <>
struct PointerLikeTypeTraits<uintptr_t> {
  static inline void *getAsVoidPointer(uintptr_t P) { return reinterpret_cast<void *>(P); }
  static inline uintptr_t getFromVoidPointer(void *P) { return reinterpret_cast<uintptr_t>(P); }
  // No bits are available!
  static constexpr int NumLowBitsAvailable = 0;
};

template <int Alignment, typename FunctionPointerT>
struct FunctionPointerLikeTypeTraits {
  static constexpr int NumLowBitsAvailable = detail::ConstantLog2<Alignment>::value;
  static inline void *getAsVoidPointer(FunctionPointerT P) {
    assert((reinterpret_cast<uintptr_t>(P) & ~((uintptr_t)-1 << NumLowBitsAvailable)) == 0 && "Alignment not satisfied for an actual function pointer!");
    return reinterpret_cast<void *>(P);
  }
  static inline FunctionPointerT getFromVoidPointer(void *P) { return reinterpret_cast<FunctionPointerT>(P); }
};
template <typename ReturnT, typename... ParamTs>
struct PointerLikeTypeTraits<ReturnT (*)(ParamTs...)> : FunctionPointerLikeTypeTraits<4, ReturnT (*)(ParamTs...)> {};

template <typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo;

template <typename PointerTy, unsigned IntBits, typename IntType = unsigned, typename PtrTraits = PointerLikeTypeTraits<PointerTy>, typename Info = PointerIntPairInfo<PointerTy, IntBits, PtrTraits>>
class PointerIntPair {
  using InfoTy = Info;
  intptr_t Value = 0;

 public:
  constexpr PointerIntPair() = default;
  PointerIntPair(PointerTy ptrValue, IntType intVal) { setPointerAndInt(ptrValue, intVal); }

  explicit PointerIntPair(PointerTy ptrValue) { initWithPointer(ptrValue); }

  PointerTy getPointer() const { return Info::getPointer(Value); }

  IntType getInt() const { return (IntType)Info::getInt(Value); }

  void setPointer(PointerTy ptrValue) { Value = Info::updatePointer(Value, ptrValue); }

  void setInt(IntType intVal) { Value = Info::updateInt(Value, static_cast<intptr_t>(intVal)); }

  void initWithPointer(PointerTy ptrValue) { Value = Info::updatePointer(0, ptrValue); }

  void setPointerAndInt(PointerTy ptrValue, IntType intVal) { Value = Info::updateInt(Info::updatePointer(0, ptrValue), static_cast<intptr_t>(intVal)); }

  PointerTy const *getAddrOfPointer() const { return const_cast<PointerIntPair *>(this)->getAddrOfPointer(); }

  PointerTy *getAddrOfPointer() {
    assert(Value == reinterpret_cast<intptr_t>(getPointer()) &&
           "Can only return the address if IntBits is cleared and "
           "PtrTraits doesn't change the pointer");
    return reinterpret_cast<PointerTy *>(&Value);
  }

  void *getOpaqueValue() const { return reinterpret_cast<void *>(Value); }

  void setFromOpaqueValue(void *val) { Value = reinterpret_cast<intptr_t>(val); }

  static PointerIntPair getFromOpaqueValue(void *v) {
    PointerIntPair p;
    p.setFromOpaqueValue(v);
    return p;
  }

  static PointerIntPair getFromOpaqueValue(const void *v) {
    (void)PtrTraits::getFromVoidPointer(v);
    return getFromOpaqueValue(const_cast<void *>(v));
  }

  bool operator==(const PointerIntPair &rhs) const { return Value == rhs.Value; }

  bool operator!=(const PointerIntPair &rhs) const { return Value != rhs.Value; }

  bool operator<(const PointerIntPair &rhs) const { return Value < rhs.Value; }
  bool operator>(const PointerIntPair &rhs) const { return Value > rhs.Value; }

  bool operator<=(const PointerIntPair &rhs) const { return Value <= rhs.Value; }

  bool operator>=(const PointerIntPair &rhs) const { return Value >= rhs.Value; }
};

template <typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo {
  static_assert(PtrTraits::NumLowBitsAvailable < std::numeric_limits<uintptr_t>::digits, "cannot use a pointer type that has all bits free");
  static_assert(IntBits <= PtrTraits::NumLowBitsAvailable, "pointerintpair with integer size too large for pointer");
  enum MaskAndShiftConstants : uintptr_t {
    PointerBitMask = ~(uintptr_t)(((intptr_t)1 << PtrTraits::NumLowBitsAvailable) - 1),
    IntShift = (uintptr_t)PtrTraits::NumLowBitsAvailable - IntBits,
    IntMask = (uintptr_t)(((intptr_t)1 << IntBits) - 1),
    ShiftedIntMask = (uintptr_t)(IntMask << IntShift)
  };
  static PointerT getPointer(intptr_t value) { return PtrTraits::getFromVoidPointer(reinterpret_cast<void *>(value & PointerBitMask)); }
  static intptr_t getInt(intptr_t value) { return (value >> IntShift) & IntMask; }
  static intptr_t updatePointer(intptr_t origValue, PointerT ptr) {
    intptr_t PtrWord = reinterpret_cast<intptr_t>(PtrTraits::getAsVoidPointer(ptr));
    assert((PtrWord & ~PointerBitMask) == 0 && "Pointer is not sufficiently aligned");
    // Preserve all low bits, just update the pointer.
    return PtrWord | (origValue & ~PointerBitMask);
  }
  static intptr_t updateInt(intptr_t origValue, intptr_t Int) {
    intptr_t IntWord = static_cast<intptr_t>(Int);
    assert((IntWord & ~IntMask) == 0 && "Integer too large for field");

    // Preserve all bits other than the ones we are updating.
    return (origValue & ~ShiftedIntMask) | IntWord << IntShift;
  }
};
template <typename PointerTy, unsigned IntBits, typename IntType, typename PtrTraits>
struct PointerLikeTypeTraits<PointerIntPair<PointerTy, IntBits, IntType, PtrTraits>> {
  static inline void *getAsVoidPointer(const PointerIntPair<PointerTy, IntBits, IntType> &p) { return p.getOpaqueValue(); }

  static inline PointerIntPair<PointerTy, IntBits, IntType> getFromVoidPointer(void *p) { return PointerIntPair<PointerTy, IntBits, IntType>::getFromOpaqueValue(p); }

  static inline PointerIntPair<PointerTy, IntBits, IntType> getFromVoidPointer(const void *p) { return PointerIntPair<PointerTy, IntBits, IntType>::getFromOpaqueValue(p); }

  static constexpr int NumLowBitsAvailable = PtrTraits::NumLowBitsAvailable - IntBits;
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_SUPPORT_TYPE_TRAITS_ */
