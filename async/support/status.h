#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_STATUS_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_STATUS_

#include <type_traits>
#include <utility>

namespace ficus {
namespace async {

template <typename T>
struct StatusAnd {
  using value_type = T;
  StatusAnd() = default;
  StatusAnd(StatusAnd&&) = default;
  StatusAnd(const StatusAnd&) = default;
  StatusAnd& operator=(const StatusAnd&) = default;
  explicit StatusAnd(int errorCode, T&& data)
      : errorCode(errorCode), mData(std::forward<T>(data)) {}
  template <typename... Args,
            std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
  explicit StatusAnd(int errorCode, Args&&... args)
      : mData(std::forward<Args>(args)...), errorCode(errorCode) {}
  template <typename... Args>
  T& emplace(Args&&... args) {
    if (ok()) {
      mData.~T();
    }
    errorCode = 0;
    new (&mData) T(std::forward<Args>(args)...);
  }
  bool ok() const {
    // 这里FICUS_SUCC=0, 为了避免在Support模块中引入ficus依赖，这里直接使用0
    return errorCode == 0;
  }
  int ErrorCode() const { return errorCode; }
  T& Data() { return mData; }
  const T& Data() const { return mData; }
  int errorCode;
  T mData;
};

template <typename T, typename... Args,
          std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
StatusAnd<T> make_status(int errorCode, Args&&... data) {
  return StatusAnd<T>(errorCode, std::forward<Args>(data)...);
}
template <typename T>
StatusAnd<T> make_status() {
  return StatusAnd<T>();
}

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_STATUS_ */
