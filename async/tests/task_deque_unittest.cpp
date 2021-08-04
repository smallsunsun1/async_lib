#include "async/concurrent/task_deque.h"

#include <chrono>
#include <deque>
#include <iostream>
#include <thread>
#include <vector>

#include "async/support/task_function.h"
#include "gtest/gtest.h"

using namespace sss;
using namespace async;
using namespace std::chrono;

static std::mutex globalMutex;

TEST(TaskQueueTool, EnPopqueueTest) {
  internal::TaskDeque mLockFreeQueue;
  std::deque<TaskFunction> mLockQueue;
  std::vector<std::thread> totalThreads;
  std::vector<std::thread> totalPopThreads;
  auto startEn1 = high_resolution_clock::now();
  int numCores = std::min(std::thread::hardware_concurrency(),
                          static_cast<unsigned int>(20));
  int numIterations = 200;
  for (int i = 0; i < numCores; ++i) {
    totalThreads.emplace_back([&mLockFreeQueue, numIterations]() {
      for (int j = 0; j < numIterations; ++j) {
        unique_function<void()> f = []() {
          std::cout << "hello world!" << std::endl;
        };
        unique_function<void()> ff = []() {
          std::cout << "hello world!" << std::endl;
        };
        mLockFreeQueue.PushFront(TaskFunction(std::move(f)));
      }
    });
  }
  for (int i = 0; i < numCores; ++i) {
    totalPopThreads.emplace_back([&mLockFreeQueue]() {
      while (!mLockFreeQueue.Empty()) {
        mLockFreeQueue.PopBack();
      }
    });
  }
  for (auto &refThread : totalThreads) {
    refThread.join();
  }
  for (auto &refThread : totalPopThreads) {
    refThread.join();
  }
  auto endEn1 = high_resolution_clock::now();
  int duration = duration_cast<microseconds>(endEn1 - startEn1).count();
  totalThreads.clear();
  totalPopThreads.clear();

  auto startEn2 = high_resolution_clock::now();
  for (int i = 0; i < numCores; ++i) {
    totalThreads.emplace_back([&mLockQueue, numIterations]() {
      for (int j = 0; j < numIterations; ++j) {
        unique_function<void()> f = []() {
          std::cout << "hello world!" << std::endl;
        };
        unique_function<void()> ff = []() {
          std::cout << "hello world!" << std::endl;
        };
        std::lock_guard<std::mutex> lock(globalMutex);
        mLockQueue.push_front(TaskFunction(std::move(f)));
      }
    });
  }
  for (int i = 0; i < numCores; ++i) {
    totalPopThreads.emplace_back([&mLockQueue, numIterations]() {
      for (int j = 0; j < numIterations; ++j) {
        std::lock_guard<std::mutex> lock(globalMutex);
        if (!mLockQueue.empty()) {
          mLockQueue.pop_front();
        }
      }
    });
  }
  for (auto &refThread : totalThreads) {
    refThread.join();
  }
  for (auto &refThread : totalPopThreads) {
    refThread.join();
  }
  auto endEn2 = high_resolution_clock::now();
  int duration2 = duration_cast<microseconds>(endEn2 - startEn2).count();
  std::cout << "lock_free algorithm time cost: " << duration << "\n"
            << "lock_based algorithm time cost: " << duration2 << std::endl;
  totalThreads.clear();
  totalPopThreads.clear();
  EXPECT_GE(duration2, duration);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
