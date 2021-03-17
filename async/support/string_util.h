#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_STRING_UTIL_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_STRING_UTIL_

#include <sstream>  // for basic_ostream::operator<<, operator<<, stringstream
#include <string>   // for string, operator<<
#include <utility>  // for forward
#include <vector>   // for vector

namespace sss {
namespace async {
std::vector<std::string> StrSplit(const std::string& s, const std::string& delimer);

// TODO(jhsun) 效率较低的实现，后续为了提升字符串操作效率需要使用新的StrCat实现
template <typename T>
void ToStreamHelper(std::stringstream& stream, T&& t) {
  stream << std::forward<T>(t);
}
template <typename T, typename... Args>
void ToStreamHelper(std::stringstream& stream, T&& t, Args&&... args) {
  stream << std::forward<T>(t);
  ToStreamHelper(stream, std::forward<Args>(args)...);
}
template <typename... Args>
std::string StrCat(Args&&... args) {
  std::stringstream stream;
  ToStreamHelper(stream, std::forward<Args>(args)...);
  return stream.str();
}

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_STRING_UTIL_ */
