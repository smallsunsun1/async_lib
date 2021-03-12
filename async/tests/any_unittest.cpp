#include "../support/any.h"

#include <iostream>

#include "gtest/gtest.h"

using namespace ficus;
using namespace async;

TEST(ANY, V1) {
  Any a = make_any<int>(2);
  Any b(3);
  int d = any_cast<int>(a);
  int e = any_cast<int>(std::move(b));
  EXPECT_EQ(d, 2);
  EXPECT_EQ(e, 3);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  (void)RUN_ALL_TESTS();
}