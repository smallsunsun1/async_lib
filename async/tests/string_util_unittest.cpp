#include "../support/string_util.h"

#include <iostream>
#include <vector>

#include "gtest/gtest.h"

TEST(STRCAT, V1) {
  std::string s = "sss";
  auto res = ficus::async::StrCat(1, 2, "123", '1', s);
  bool value = (res == "121231sss");
  EXPECT_TRUE(value);
  std::vector<std::string> strVec = ficus::async::StrSplit(res, "1");
  std::vector<std::string> label = {"", "2", "23", "sss"};
  EXPECT_EQ(strVec, label);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  (void)RUN_ALL_TESTS();
}