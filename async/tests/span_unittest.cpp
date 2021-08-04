#include "async/support/span.h"

#include "gtest/gtest.h"

using namespace sss;
using namespace async;

TEST(SPAN, Container) {
  std::vector<int> vec = {1, 2, 3, 4, 5};
  span<int> sp1 = MakeSpan(vec);
  for (size_t i = 0; i < sp1.size(); ++i) {
    EXPECT_EQ(sp1[i], i + 1);
  }
  int a[5] = {1, 2, 3, 4, 5};
  span<int> sp2 = MakeSpan(a);
  for (size_t i = 0; i < sp2.size(); ++i) {
    EXPECT_EQ(sp2[i], i + 1);
  }
}

TEST(SPAN, Copy) {
  std::vector<int> vec = {1, 2, 3, 4, 5};
  span<int> sp1 = MakeSpan(vec);
  auto sp2 = sp1;
  EXPECT_TRUE(sp2 == sp1);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
