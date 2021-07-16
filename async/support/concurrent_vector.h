#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

namespace sss {
namespace async {

template <typename T>
class ConcurrentVector {
 public:
  // Initialize the vector with the given initial_capapcity
  explicit ConcurrentVector(size_t initial_capacity) : mState(0ull) {
    // ConcurrentVector does not support inserting more than 2^64 elements,
    // which should be more than enough for any reasonable use case.
    mAllAllocatedElements.reserve(65);
    mAllAllocatedElements.emplace_back();
    auto &v = mAllAllocatedElements.back();
    v.reserve(std::max(static_cast<size_t>(1), initial_capacity));
  }

  T &operator[](size_t index) {
    auto state = State::Decode(mState.load(std::memory_order_acquire));
    assert(index < state.size);
    return mAllAllocatedElements[state.mLastAllocated][index];
  }

  const T &operator[](size_t index) const {
    auto state = State::Decode(mState.load(std::memory_order_acquire));
    assert(index < state.size);
    return mAllAllocatedElements[state.mLastAllocated][index];
  }

  // Return the number of elements currently valid in this vector.  The vector
  // only grows, so this is conservative w.r.t. the execution of the current
  // thread.
  size_t size() const {
    return State::Decode(mState.load(std::memory_order_relaxed)).size;
  }

  // Insert a new element at the end. If the current buffer is full, we allocate
  // a new buffer with twice as much capacity and copy the items in the
  // previous buffer over.
  //
  // Returns the index of the newly inserted item.
  template <typename... Args>
  size_t emplace_back(Args &&... args) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto &last = mAllAllocatedElements.back();

    if (last.size() < last.capacity()) {
      // There is still room in the current vector without reallocation. Just
      // add the new element there.
      last.emplace_back(std::forward<Args>(args)...);

      // Increment the size of the concurrent vector.
      auto state = State::Decode(mState.load(std::memory_order_relaxed));
      state.size += 1;
      mState.store(state.Encode(), std::memory_order_release);

      return state.size - 1;  // return insertion index
    }
    // There is no more room in the current vector without reallocation.
    // Allocate a new vector with twice as much capacity, copy the elements
    // from the previous vector, and set elements_ to point to the data of the
    // new vector.
    mAllAllocatedElements.emplace_back();
    auto &newLast = mAllAllocatedElements.back();
    auto &prev = *(mAllAllocatedElements.rbegin() + 1);
    newLast.reserve(prev.capacity() * 2);
    assert(prev.size() == prev.capacity());

    // Copy over the previous vector to the new vector.
    newLast.insert(newLast.begin(), prev.begin(), prev.end());
    newLast.emplace_back(std::forward<Args>(args)...);

    // Increment the size of the concurrent vector and index of the last
    // allocated vector.
    auto state = State::Decode(mState.load(std::memory_order_relaxed));
    state.mLastAllocated += 1;
    state.size += 1;
    mState.store(state.Encode(), std::memory_order_release);

    return state.size - 1;  // return insertion index
  }

 private:
  // Concurrent vector state layout:
  // - Low 32 bits encode the index of the last allocated vector.
  // - High 32 bits encode the size of the concurrent vector.
  static constexpr uint64_t kLastAllocatedMask = (1ull << 32) - 1;
  static constexpr uint64_t kSizeMask = ((1ull << 32) - 1) << 32;

  struct State {
    uint64_t mLastAllocated;  // index of last allocated vector
    uint64_t size;            // size of the concurrent vector

    static State Decode(uint64_t state) {
      uint64_t mLastAllocated = (state & kLastAllocatedMask);
      uint64_t size = (state & kSizeMask) >> 32;
      return {mLastAllocated, size};
    }

    uint64_t Encode() const { return (size << 32) | mLastAllocated; }
  };

  // Stores/loads to/from this atomic used to enforce happens-before
  // relationship between emplace_back and operator[].
  std::atomic<uint64_t> mState;

  std::mutex mMutex;
  std::vector<std::vector<T>> mAllAllocatedElements;
};

}  // namespace async
}  // namespace sss