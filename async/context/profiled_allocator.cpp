#include <inttypes.h>

#include <atomic>
#include <cstdlib>

#include "async/context/host_allocator.h"

namespace sss {
namespace async {

namespace {
template <typename T>
void AtomicUpdateMax(T const &value, std::atomic<T> *maxValue) noexcept {
  T prevMaxValue = *maxValue;
  while (prevMaxValue < value &&
         !maxValue->compare_exchange_weak(prevMaxValue, value)) {
  }
}
}  // namespace

class ProfiledAllocator : public HostAllocator {
 public:
  explicit ProfiledAllocator(std::unique_ptr<HostAllocator> allocator)
      : mAllocator(std::move(allocator)) {}
  ~ProfiledAllocator() override {
    if (mPrintProfile) PrintStats();
  }
  void *AllocateBytes(size_t size, size_t alignment) override {
    ++mCurNumAllocations;
    ++mCumNumAllocations;
    mCurrBytesAllocated.fetch_add(size);
    AtomicUpdateMax<int64_t>(mCurNumAllocations, &mMaxNumAllocations);
    AtomicUpdateMax<int64_t>(mCurrBytesAllocated, &mMaxBytesAllocated);
    return mAllocator->AllocateBytes(size, alignment);
  }
  void DeallocateBytes(void *ptr, size_t size) override {
    --mCurNumAllocations;
    mCurrBytesAllocated.fetch_sub(size);
    mAllocator->DeallocateBytes(ptr, size);
  }

 protected:
  void PrintStats() const {
    printf("HostAllocator profile:\n");
    printf("Current number of allocations = %" PRId64 "\n",
           mCurNumAllocations.load());
    printf("Max number of allocations = %" PRId64 "\n",
           mMaxNumAllocations.load());
    printf("Total number of allocations = %" PRId64 "\n",
           mCumNumAllocations.load());
    printf("Current number of bytes allocated = %" PRId64 "\n",
           mCurrBytesAllocated.load());
    printf("Max number of bytes allocated = %" PRId64 "\n",
           mMaxBytesAllocated.load());
    fflush(stdout);
  }
  bool mPrintProfile = true;
  std::atomic<int64_t> mCurNumAllocations{0};
  std::atomic<int64_t> mMaxNumAllocations{0};
  std::atomic<int64_t> mCumNumAllocations{0};
  std::atomic<int64_t> mCurrBytesAllocated{0};
  std::atomic<int64_t> mMaxBytesAllocated{0};

 private:
  std::unique_ptr<HostAllocator> mAllocator;
};

class LeakCheckAllocator : public ProfiledAllocator {
 public:
  explicit LeakCheckAllocator(std::unique_ptr<HostAllocator> allocator)
      : ProfiledAllocator(std::move(allocator)) {
    mPrintProfile = false;
  }
  ~LeakCheckAllocator() override {
    if (mCurrBytesAllocated.load() != 0) {
      PrintStats();
      printf("Memory leak detected: %" PRId64 " alive allocations, %" PRId64
             " alive bytes\n",
             mCurNumAllocations.load(), mCurrBytesAllocated.load());
      fflush(stdout);
      exit(1);
    }
  }
};

std::unique_ptr<HostAllocator> CreateProfiledAllocator(
    std::unique_ptr<HostAllocator> allocator) {
  return std::unique_ptr<HostAllocator>(
      new ProfiledAllocator(std::move(allocator)));
}

}  // namespace async
}  // namespace sss