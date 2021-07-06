#include "async/context/resource.h"

#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult

#include <memory>    // for unique_ptr
#include <optional>  // for optional
#include <string>    // for string

#include "async/support/type_traits.h"  // for async, sss
#include "gtest/gtest_pred_impl.h"      // for Test, InitGoogleTest, RUN_...

using namespace sss;
using namespace async;

static const std::string res1 = "123";
static const std::string res2 = "sss";

TEST(RESOURCE, V1) {
  ResourceManager manager;
  manager.CreateResource<std::string>(res1, std::string{"123"});
  manager.CreateResource<std::string>(res2, std::string{"sss"});
  std::string s1 = (*manager.GetResource<std::string>(res1).value());
  std::string s2 = (*manager.GetResource<std::string>(res2).value());
  EXPECT_EQ(s1, std::string{"123"});
  EXPECT_EQ(s2, std::string{"sss"});
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
