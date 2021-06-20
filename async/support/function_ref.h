#ifndef ASYNC_SUPPORT_FUNCTION_REF_
#define ASYNC_SUPPORT_FUNCTION_REF_

#include <cstddef>

#include "async/support/type_traits.h"

namespace sss {
namespace async {

template <typename Func>
class function_ref;

template <typename Ret, typename... Params>
class function_ref<Ret(Params...)> {
 public:
  function_ref() = default;
  template <typename Callable>
  function_ref(Callable &&callable,
               // This is not the copy-constructor.
               std::enable_if_t<!std::is_same<remove_cv_ref_t<Callable>, function_ref>::value> * = nullptr,
               // Functor must be callable and return a suitable type.
               std::enable_if_t<std::is_void<Ret>::value || std::is_convertible<decltype(std::declval<Callable>()(std::declval<Params>()...)), Ret>::value> * = nullptr)
      : callback(callback_fn<typename std::remove_reference<Callable>::type>), callable(reinterpret_cast<intptr_t>(&callable)) {}

  Ret operator()(Params... params) const { return callback(callable, std::forward<Params>(params)...); }

  explicit operator bool() const { return callback; }

 private:
  intptr_t callable;
  Ret (*callback)(intptr_t callee, Params... params) = nullptr;
  template <typename Callable>
  static Ret callback_fn(intptr_t callable, Params... params) {
    return (*reinterpret_cast<Callable *>(callable))(std::forward<Params>(params)...);
  }
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_SUPPORT_FUNCTION_REF_ */
