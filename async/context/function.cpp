#include "function.h"

namespace ficus {
namespace async {

void Function::Execute(const Function* func, std::vector<RCReference<AsyncValue>>& arguments, std::vector<RCReference<AsyncValue>>& results, HostContext* ctx) {
    absl::InlinedVector<AsyncValue*, 4> argumentsPtr;
    argumentsPtr.reserve(arguments.size());
    for (auto& elem: arguments) {
        argumentsPtr.push_back(elem.get());
    }
    func->Execute(absl::MakeConstSpan(argumentsPtr.data(), argumentsPtr.size()), absl::MakeSpan(results.data(), results.size()), ctx);
}

}
}