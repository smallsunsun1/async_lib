#include <omp.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <thread>

#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/runtime/task_graph.h"
#include "async/support/ref_count.h"
#include "gtest/gtest.h"

using namespace sss;

double OrderedPrintTest(uint32_t numCount) {
  auto start = std::chrono::high_resolution_clock::now();
#pragma omp parallel
  {
#pragma omp for ordered
    for (uint32_t i = 0; i < numCount; ++i) {
#pragma omp ordered
      { std::printf("%d\n", i); }
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  return duration;
}

double AsyncOrderedPrintTest(uint32_t numCount, async::HostContext *context) {
  std::vector<async::RCReference<async::AsyncValue>> container;
  container.resize(numCount);
  for (uint32_t i = 0; i < numCount; ++i) {
    container[i] = context->MakeUnconstructedAsyncValueRef<async::Chain>();
  }
  container[0]->emplace<async::Chain>();
  auto start = std::chrono::high_resolution_clock::now();
  for (uint32_t i = 1; i < numCount; ++i) {
    context->RunWhenReady(absl::MakeConstSpan(container.data() + i - 1, 1),
                          [i, &container, context]() {
                            std::printf("%d\n", i);
                            container[i]->emplace<async::Chain>();
                          });
  }
  context->Quiesce();
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
      .count();
}

static constexpr uint32_t kNumCount = 10000;

TEST(ASYNC, OPENMP_ASYNC) {
  omp_set_num_threads(std::thread::hardware_concurrency());
  std::unique_ptr<async::HostContext> context =
      async::CreateCustomHostContext(std::thread::hardware_concurrency(), 1);
  double time1 = OrderedPrintTest(kNumCount);
  double time2 = AsyncOrderedPrintTest(kNumCount, context.get());
  EXPECT_TRUE(time2 < 10 * time1);
  std::cout << "time openmp: " << time1 << std::endl;
  std::cout << "time async: " << time2 << std::endl;
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
