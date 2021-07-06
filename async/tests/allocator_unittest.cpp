#include "async/context/host_allocator.h"
#include "gtest/gtest.h"

namespace sss {

class AllocatorTest : public testing::Test {
 protected:
  void SetUp() override final {
    mBaseAllocator = async::CreateMallocAllocator();
    mProfiledAllocator =
        async::CreateProfiledAllocator(async::CreateMallocAllocator());
  }
  void TearDown() override final {}
  std::unique_ptr<async::HostAllocator> mBaseAllocator;
  std::unique_ptr<async::HostAllocator> mProfiledAllocator;
};

TEST_F(AllocatorTest, V1) {
  int *mem = mProfiledAllocator->Allocate<int>();
  mProfiledAllocator->Deallocate(mem);
}

}  // namespace sss

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}