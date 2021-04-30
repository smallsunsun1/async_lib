#include <omp.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

void LoopSimdTest() {
  std::vector<int> data(100);
#pragma omp parallel for simd
  for (int i = 0; i < 100; ++i) {
    data[i] = i + i;
  }
  std::cout << std::accumulate(data.begin(), data.end(), 0) << std::endl;
}

void LoopCollapseTest() {
  int result = 0;
#pragma omp parallel for collapse(2) reduction(+:result) 
  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < 100; ++j) {
      result += i;
    }
  }
  std::cout << result << std::endl;
}

void ParallelTaskTest() {
#pragma omp parallel 
{
#pragma omp single
for (int i = 0; i < 10; ++i) {
#pragma omp task
{
  std::printf("%d\n", i);
}
}
}
}

int main() {
  omp_set_num_threads(4);
  LoopCollapseTest();
  LoopSimdTest();
  ParallelTaskTest();
}