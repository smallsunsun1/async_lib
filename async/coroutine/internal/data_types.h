#ifndef ASYNC_COROUTINE_INTERNAL_DATA_TYPES_
#define ASYNC_COROUTINE_INTERNAL_DATA_TYPES_

namespace sss {
namespace async {

struct AnyData {
  AnyData() {}
  template <typename T>
  AnyData(T&&) {}
};

}  // namespace async
}  // namespace sss

#endif /* ASYNC_COROUTINE_INTERNAL_DATA_TYPES_ */
