#ifndef ASYNC_COROUTINE_INTERNAL_DATA_TYPES_
#define ASYNC_COROUTINE_INTERNAL_DATA_TYPES_

namespace sss {
namespace async {

struct AnyData {
    template <typename T>
    AnyData(T&&) {}
};

}
}

#endif /* ASYNC_COROUTINE_INTERNAL_DATA_TYPES_ */
