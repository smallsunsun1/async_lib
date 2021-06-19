#include "async/support/string_util.h"

#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult

#include <vector>  // for vector

#include "gtest/gtest_pred_impl.h"  // for Test, AssertionResult, InitGoogle...

TEST(STRCAT, V1) {
  std::string s = "sss";
  auto res = sss::async::StrCat(1, 2, "123", '1', s);
  bool value = (res == "121231sss");
  EXPECT_TRUE(value);
  std::vector<std::string> strVec = sss::async::StrSplit(res, "1");
  std::vector<std::string> label = {"", "2", "23", "sss"};
  EXPECT_EQ(strVec, label);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}