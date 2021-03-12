#include "../context/resource.h"

#include <iostream>

#include "gtest/gtest.h"

using namespace ficus;
using namespace async;

static const std::string res1 = "123";
static const std::string res2 = "sss";

TEST(RESOURCE, V1) {
  ResourceManager manager;
  manager.CreateResource<std::string>(res1, "123");
  manager.CreateResource<std::string>(res2, "sss");
  std::string s1 = (*manager.GetResource<std::string>(res1).getValue());
  std::string s2 = (*manager.GetResource<std::string>(res2).getValue());
  EXPECT_EQ(s1, std::string{"123"});
  EXPECT_EQ(s2, std::string{"sss"});
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  (void)RUN_ALL_TESTS();
}
