#include "alloc.h"

#include <cstdlib>

namespace ficus {
namespace async {
void* AlignedAlloc(size_t alignment, size_t size) {
  return aligned_alloc(alignment, size);
}

}  // namespace async
}  // namespace ficus