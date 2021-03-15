#include "string_util.h"

#include <regex>

namespace ficus {
std::vector<std::string> async::StrSplit(const std::string& s, const std::string& delimer) {
  std::regex re(delimer);
  std::vector<std::string> v(std::sregex_token_iterator(s.begin(), s.end(), re, -1), std::sregex_token_iterator());
  return v;
}
}  // namespace ficus