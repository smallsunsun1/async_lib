#include "async/support/alloc.h"

#include <cstddef>
#include <cstdlib>

namespace sss {
namespace async {
void *AlignedAlloc(size_t alignment, size_t size) {
#ifdef _WIN32
  return _aligned_malloc(size, alignment);
#else
  return std::aligned_alloc(alignment, size);
#endif
}

}  // namespace async
}  // namespace sss