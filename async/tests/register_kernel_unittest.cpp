#include <gtest/gtest.h>

#include "async/runtime/register.h"

using namespace sss;
using namespace async;

static int value = 0;

ASYNC_STATIC_KERNEL_REGISTRATION("sss", [](CommonAsyncKernelFrame* kernel) {
  (void)kernel;
  value += 1;
  std::cout << "hello world" << std::endl;
});

TEST(STATIC_REGISTER, FUNC_TEST) {
  GET_KERNEL_FN("sss").value()(nullptr);
  EXPECT_EQ(value, 1);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return 0;
}