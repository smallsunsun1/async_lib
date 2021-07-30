#include "async/concurrent/task_priority_queue.h"

#include "gtest/gtest.h"

using namespace sss;
using namespace async;

TEST(TASK_PRIORITY_QUEUE, INPOPTEST) {
  TaskPriorityDeque queue;
  int value1 = 0;
  int value2 = 0;
  TaskFunction func1([&value1]() {
    std::cout << "func1 called!" << std::endl;
    value1 = 1;
  });
  TaskFunction func2([&value2]() {
    std::cout << "func2 called!" << std::endl;
    value2 = 2;
  });
  queue.PushFront(std::move(func2), TaskPriority::kDefault);
  queue.PushFront(std::move(func1), TaskPriority::kCritical);
  queue.PopFront().value()();
  EXPECT_EQ(value1, 1);
  queue.PopFront().value()();
  EXPECT_EQ(value2, 2);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
