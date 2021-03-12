#include "../support/latch.h"

#include <atomic>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"

using namespace ficus;
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
  (void)RUN_ALL_TESTS();
}
