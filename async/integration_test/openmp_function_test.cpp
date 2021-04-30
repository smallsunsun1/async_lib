#include <omp.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

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
#pragma omp parallel for collapse(2) reduction(+ : result)
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
      { std::printf("%d\n", i); }
    }
  }
}

void TaskDependTest() {
  int x = 0;
  int y = 5;
#pragma omp parallel
  {
#pragma omp single nowait
    {
#pragma omp task depend(out : x)
      { x = 1; }
#pragma omp task depend(in : x) depend(out : y)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        y = y + 1;
      }
#pragma omp task depend(inout : x)
      {
        x++;
        assert(x == 2);
      }
#pragma omp task depend(in : x, y)
      { std::cout << x + y << std::endl; }
    }
  }
}

int main() {
  omp_set_num_threads(4);
  std::cout << "LoopCollapseTest Start" << std::endl;
  LoopCollapseTest();
  std::cout << "LoopSimdTest Start" << std::endl;
  LoopSimdTest();
  std::cout << "ParallelTaskTest Start" << std::endl;
  ParallelTaskTest();
  std::cout << "TaskDependTest Start" << std::endl;
  TaskDependTest();
}