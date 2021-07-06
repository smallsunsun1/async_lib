#include "async/support/unique_function.h"

#include <iostream>

#include "gtest/gtest.h"

using namespace sss;
using namespace async;

TEST(UNQIUE_FUNCTION, V1) {
  int result = 0;
  auto f = [a = 3, &result]() {
    std::cout << "Hello World!" << std::endl;
    result += a;
  };
  unique_function<void()> ff(f);
  ff();
  unique_function<void()> fff(std::move(ff));
  fff();
  EXPECT_EQ(result, 6);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}