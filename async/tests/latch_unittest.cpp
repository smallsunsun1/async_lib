#include "async/support/latch.h"

#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult

#include <atomic>  // for atomic
#include <thread>  // for thread

#include "gtest/gtest_pred_impl.h"  // for Test, InitGoogleTest, RUN_ALL_TESTS

using namespace sss;
using namespace async;

TEST(LATCH, V1) {
  latch t(5);
  std::atomic<int> value{0};
  for (int i = 0; i < 5; ++i) {
    std::thread thread([&t, &value]() {
      value.fetch_add(1);
      t.count_down();
    });
    thread.detach();
  }
  t.wait();
  EXPECT_EQ(value.load(), 5);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
