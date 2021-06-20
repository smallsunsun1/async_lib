#include "async/support/function_ref.h"

#include "gtest/gtest.h"

using namespace sss;
using namespace async;

TEST(FUNCTION_REF, V1) {
  int result = 0;
  auto f = [a = 3, &result]() {
    std::cout << "Hello World!" << std::endl;
    result += a;
  };
  function_ref<void()> ff(f);
  ff();
  function_ref<void()> fff(std::move(ff));
  fff();
  EXPECT_EQ(result, 6);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}