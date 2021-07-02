#include "async/concurrent/concurrent_work_queue.h"

#include <string_view>
#include <unordered_map>

#include "absl/container/flat_hash_map.h"

namespace sss {
namespace async {
namespace {
using WorkQueueFactoryMap = absl::flat_hash_map<std::string_view, WorkQueueFactory>;
WorkQueueFactoryMap* GetWorkQueueFactories() {
  static WorkQueueFactoryMap* factories = new WorkQueueFactoryMap;
  return factories;
}
}  // namespace

ConcurrentWorkQueue::~ConcurrentWorkQueue() = default;
void RegisterWorkQueueFactory(std::string_view name, WorkQueueFactory factory) {
  auto p = GetWorkQueueFactories()->try_emplace(name, std::move(factory));
  (void)p;
  assert(p.second && "Factory already registered");
}
std::unique_ptr<ConcurrentWorkQueue> CreateWorkQueue(std::string_view config) {
  size_t colon = config.find(':');
  std::string_view name = colon == std::string_view::npos ? config : config.substr(0, colon);
  std::string_view args = colon == std::string_view::npos ? "" : config.substr(colon + 1);
  WorkQueueFactoryMap* factories = GetWorkQueueFactories();
  auto it = factories->find(name);
  if (it == factories->end()) {
    return nullptr;
  }
  return it->second(args);
}

}  // namespace async
}  // namespace sss