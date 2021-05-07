#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_RESOURCE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_RESOURCE_

#include <assert.h>  // for assert

#include <memory>
#include <mutex>  // for mutex
#include <unordered_map>
#include <utility>  // for for...

#include "async/support/type_traits.h"
#include "absl/container/flat_hash_map.h"  // for fla...
#include "absl/strings/string_view.h"      // for str...
#include "absl/types/any.h"                // for any...
#include "absl/types/optional.h"           // for nul...

namespace sss {
namespace async {
class ResourceManager {
 public:
  ResourceManager() = default;
  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;
  template <typename T>
  T* MustGetResource(absl::string_view name) {
    std::lock_guard<std::mutex> lock(mMu);
    auto iter = mResource.find(name);
    assert(iter != mResource.end());
    T* data = absl::any_cast<T>(&iter->second);
    return data;
  }
  template <typename T>
  absl::optional<T*> GetResource(absl::string_view name) {
    std::lock_guard<std::mutex> lock(mMu);
    auto iter = mResource.find(name);
    if (iter == mResource.end()) return absl::nullopt;
    T* data = absl::any_cast<T>(&iter->second);
    return data;
  }
  template <typename T, typename... Args>
  T* GetOrCreateResource(absl::string_view name, Args&&... args) {
    std::lock_guard<std::mutex> lock(mMu);
    auto res = mResource.try_emplace(name, absl::make_any<T>(std::forward<Args>(args)...));
    return absl::any_cast<T>(&res.first->second);
  }
  template <typename T, typename... Args>
  T* CreateResource(absl::string_view name, Args&&... args) {
    std::lock_guard<std::mutex> lock(mMu);
    auto res = mResource.try_emplace(name, absl::make_any<T>(std::forward<Args>(args)...));
    return absl::any_cast<T>(&res.first->second);
  }
  void DeleteResource(absl::string_view name) {
    std::lock_guard<std::mutex> lock(mMu);
    auto iter = mResource.find(name);
    if (iter == mResource.end()) {
      return;
    }
    mResource.erase(iter);
  }

 private:
  std::mutex mMu;
  absl::flat_hash_map<absl::string_view, absl::any> mResource;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_RESOURCE_ */
