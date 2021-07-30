#include "async/support/alloc.h"

#include <cstdlib>
#include <cstddef>

namespace sss {
namespace async {
void *AlignedAlloc(size_t alignment, size_t size) {
  return aligned_alloc(alignment, size);
}

}  // namespace async
}  // namespace sss