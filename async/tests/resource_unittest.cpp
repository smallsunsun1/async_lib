#include "../context/resource.h"

#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult

#include <memory>  // for unique_ptr
#include <string>  // for string

#include "absl/container/flat_hash_map.h"  // for BitMask
#include "absl/types/optional.h"           // for optional
#include "async/support/type_traits.h"     // for async, ficus
#include "gtest/gtest_pred_impl.h"         // for Test, InitGoogleTest, RUN_...

using namespace ficus;
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

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
