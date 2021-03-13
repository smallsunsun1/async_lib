#include "async/runtime/graph.h"
#include "async/context/kernel_frame.h"
#include "async/context/host_context.h"

using namespace ficus;
using namespace async;

int main() {
    auto runContext = CreateCustomHostContext(20, 1);
}