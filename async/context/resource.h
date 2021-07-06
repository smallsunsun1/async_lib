#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_RESOURCE_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_RESOURCE_

#include <any>
#include <cassert>
#include <memory>
#include <mutex>        // for mutex
#include <optional>     // for nul...
#include <string_view>  // for str...
#include <unordered_map>
#include <utility>  // for for...

#include "async/support/type_traits.h"

namespace sss {
namespace async {
class ResourceManager {
 public:
  ResourceManager() = default;
  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;
  template <typename T>
  T *MustGetResource(std::string_view name) {
    std::lock_guard<std::mutex> lock(mMu);
    auto iter = mResource.find(name);
    assert(iter != mResource.end());
    T *data = std::any_cast<T>(&iter->second);
    return data;
  }
  template <typename T>
  std::optional<T *> GetResource(std::string_view name) {
    std::lock_guard<std::mutex> lock(mMu);
    auto iter = mResource.find(name);
    if (iter == mResource.end()) return std::nullopt;
    T *data = std::any_cast<T>(&iter->second);
    return data;
  }
  template <typename T, typename... Args>
  T *GetOrCreateResource(std::string_view name, Args &&...args) {
    std::lock_guard<std::mutex> lock(mMu);
    auto res = mResource.try_emplace(
        name, std::make_any<T>(std::forward<Args>(args)...));
    return std::any_cast<T>(&res.first->second);
  }
  template <typename T, typename... Args>
  T *CreateResource(std::string_view name, Args &&...args) {
    std::lock_guard<std::mutex> lock(mMu);
    auto res = mResource.try_emplace(
        name, std::make_any<T>(std::forward<Args>(args)...));
    return std::any_cast<T>(&res.first->second);
  }
  void DeleteResource(std::string_view name) {
    std::lock_guard<std::mutex> lock(mMu);
    auto iter = mResource.find(name);
    if (iter == mResource.end()) {
      return;
    }
    mResource.erase(iter);
  }

 private:
  std::mutex mMu;
  std::unordered_map<std::string_view, std::any> mResource;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_CONTEXT_RESOURCE_ */
