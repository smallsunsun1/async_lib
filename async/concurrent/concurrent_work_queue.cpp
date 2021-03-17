#include "concurrent_work_queue.h"

#include <unordered_map>

#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"

namespace sss {
namespace async {
namespace {
using WorkQueueFactoryMap = absl::flat_hash_map<absl::string_view, WorkQueueFactory>;
WorkQueueFactoryMap* GetWorkQueueFactories() {
  static WorkQueueFactoryMap* factories = new WorkQueueFactoryMap;
  return factories;
}
}  // namespace

ConcurrentWorkQueue::~ConcurrentWorkQueue() = default;
void RegisterWorkQueueFactory(absl::string_view name, WorkQueueFactory factory) {
  auto p = GetWorkQueueFactories()->try_emplace(name, std::move(factory));
  (void)p;
  assert(p.second && "Factory already registered");
}
std::unique_ptr<ConcurrentWorkQueue> CreateWorkQueue(absl::string_view config) {
  size_t colon = config.find(':');
  absl::string_view name = colon == absl::string_view::npos ? config : config.substr(0, colon);
  absl::string_view args = colon == absl::string_view::npos ? "" : config.substr(colon + 1);
  WorkQueueFactoryMap* factories = GetWorkQueueFactories();
  auto it = factories->find(name);
  if (it == factories->end()) {
    return nullptr;
  }
  return it->second(args);
}

}  // namespace async
}  // namespace sss