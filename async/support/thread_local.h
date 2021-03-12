#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_THREAD_LOCAL_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_THREAD_LOCAL_

#include <cstddef>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "third_party/abseil-cpp/absl/functional/function_ref.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ficus {
namespace async {

namespace internal {
template <typename T>
struct DefaultConstructor {
  T Construct() { return T(); }
};
}  // namespace internal

template <typename T, typename Constructor = internal::DefaultConstructor<T>>
class ThreadLocal {
 public:
  // 计算预期访问ThreadLocal的现有线程数的最佳容量。只要负载因子小于1，查找操作的预期时间是恒定的。
  // 参考链接: https://en.wikipedia.org/wiki/Linear_probing#Analysis
  static size_t Capacity(size_t numThreads) { return numThreads * 2; }

  template <typename... Args>
  explicit ThreadLocal(size_t capacity, Args... args)
      : mCapacity(capacity),
        mConstructor(std::forward<Args>(args)...),
        mNumLockFreeEntries(0) {
    assert(mCapacity >= 0);
    mData.resize(capacity);
    mPtrs = new std::atomic<Entry*>[mCapacity];
    for (int i = 0; i < mCapacity; ++i) {
      mPtrs[i].store(nullptr, std::memory_order_relaxed);
    }
  }

  ~ThreadLocal() { delete[] mPtrs; }

  T& Local() {
    std::thread::id this_thread = std::this_thread::get_id();
    if (mCapacity == 0) return SpilledLocal(this_thread);

    size_t h = std::hash<std::thread::id>()(this_thread);
    const int start_idx = h % mCapacity;

    // 注意：根据“std:：this_thread:：get_id（）”的定义，
    // 可以保证我们永远不能使用相同的线程id同时调用此函数。
    // 如果在初始遍历期间没有找到条目，则可以保证没有其他人可以同时插入它。

    // 检查是否已经拥有了this_thread这个thread
    int idx = start_idx;
    while (mPtrs[idx].load(std::memory_order_acquire) != nullptr) {
      Entry& entry = *(mPtrs[idx].load());
      if (entry.thread_id == this_thread) return entry.value;

      idx += 1;
      if (idx >= mCapacity) idx -= mCapacity;
      if (idx == start_idx) break;
    }

    // 如果无锁存储满了，使用lock形式的存储结果
    if (mNumLockFreeEntries.load(std::memory_order_relaxed) >= mCapacity)
      return SpilledLocal(this_thread);

    int insertionIndex =
        mNumLockFreeEntries.fetch_add(1, std::memory_order_relaxed);
    if (insertionIndex >= mCapacity) return SpilledLocal(this_thread);

    // 保证在没有竞争的情况下去除mData[insertionIndex]
    mData[insertionIndex] = {this_thread, mConstructor.Construct()};

    // 找到结果指针
    Entry* inserted = mData[insertionIndex].getPointer();

    Entry* empty = nullptr;

    // Now we have to find an insertion point into the lookup table. We start
    // from the `idx` that was identified as an insertion point above, it's
    // guaranteed that we will have an empty entry somewhere in a lookup table
    // (because we created an entry in the `mData`).
    const int insertionIdx = idx;

    do {
      // Always start search from the original insertion candidate.
      idx = insertionIdx;
      while (mPtrs[idx].load(std::memory_order_relaxed) != nullptr) {
        idx += 1;
        if (idx >= mCapacity) idx -= mCapacity;
        // If we did a full loop, it means that we don't have any free entries
        // in the lookup table, and this means that something is terribly wrong.
        assert(idx != insertionIdx);
      }
      // Atomic store of the pointer guarantees that any other thread, that will
      // follow this pointer will see all the mutations in the `mData`.
    } while (!mPtrs[idx].compare_exchange_weak(empty, inserted,
                                               std::memory_order_release));

    return inserted->value;
  }

  // WARN: It's not thread safe to call it concurrently with `local()`.
  void ForEach(absl::FunctionRef<void(std::thread::id, T&)> f) {
    // Reading directly from `mData` is unsafe, because only store to the
    // entry in `mPtrs` makes all changes visible to other threads.
    for (int i = 0; i < mCapacity; ++i) {
      Entry* entry = mPtrs[i].load(std::memory_order_acquire);
      if (entry == nullptr) continue;
      f(entry->thread_id, entry->value);
    }

    // We did not spill into the map based storage.
    if (mNumLockFreeEntries.load(std::memory_order_relaxed) < mCapacity) return;

    std::lock_guard<std::mutex> lock(mMu);
    for (auto& kv : mSpilled) {
      f(kv.first, kv.second);
    }
  }

 private:
  struct Entry {
    std::thread::id thread_id;
    T value;
  };

  // Use synchronized unordered_map when lock-free storage is full.
  T& SpilledLocal(std::thread::id this_thread) {
    std::lock_guard<std::mutex> lock(mMu);

    auto it = mSpilled.find(this_thread);
    if (it != mSpilled.end()) return it->second;

    auto inserted = mSpilled.emplace(this_thread, mConstructor.Construct());
    assert(inserted.second);
    return (*inserted.first).second;
  }

  const size_t mCapacity;
  Constructor mConstructor;

  // Storage that backs lock-free lookup table `mPtrs`. Entries stored
  // contiguously starting from index 0. Entries stored as optional so that
  // `T` can be non-default-constructible.
  std::vector<absl::optional<Entry>> mData;

  // Atomic pointers to the data stored in `mData`. Used as a lookup table for
  // linear probing hash map (https://en.wikipedia.org/wiki/Linear_probing).
  std::atomic<Entry*>* mPtrs;

  // Number of entries stored in the lock free storage.
  std::atomic<int> mNumLockFreeEntries;

  // When the lock-free storage is full we spill to the unordered map
  // synchronized with a mutex. In practice this should never happen, if
  // `mCapacity` is a reasonable estimate of the number of unique threads that
  // are expected to access this instance of ThreadLocal.
  std::mutex mMu;
  std::unordered_map<std::thread::id, T> mSpilled;
};

}  // namespace async
}  // namespace ficus

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_SUPPORT_THREAD_LOCAL_ */
