#ifndef ASYNC_COROUTINE_INTERNAL_DATA_TYPES_
#define ASYNC_COROUTINE_INTERNAL_DATA_TYPES_

#include <exception>
namespace sss {
namespace async {

struct AnyData {
  AnyData() {}
  template <typename T>
  AnyData(T&&) {}
};

class BadPromise: public std::exception {
public:
  virtual const char* what() const override  {
    return "Bad Promise";
  }
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_INTERNAL_DATA_TYPES_ */
