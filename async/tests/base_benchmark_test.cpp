#include <benchmark/benchmark.h>

#include <array>

static constexpr int len = 6;

constexpr auto MyPow(const int i) { return i * i; }

// 使用operator[]读取元素，依次存入1-6的平方
static void BenchArrayOperator(benchmark::State& state) {
  std::array<int, len> arr;
  constexpr int i = 1;
  for (auto _ : state) {
    arr[0] = MyPow(i);
    arr[1] = MyPow(i + 1);
    arr[2] = MyPow(i + 2);
    arr[3] = MyPow(i + 3);
    arr[4] = MyPow(i + 4);
    arr[5] = MyPow(i + 5);
  }
}
BENCHMARK(BenchArrayOperator);

static void BenchArrayAt(benchmark::State& state) {
  std::array<int, len> arr;
  constexpr int i = 1;
  for (auto _ : state) {
    arr.at(0) = MyPow(i);
    arr.at(1) = MyPow(i + 1);
    arr.at(2) = MyPow(i + 2);
    arr.at(3) = MyPow(i + 3);
    arr.at(4) = MyPow(i + 4);
    arr.at(5) = MyPow(i + 5);
  }
}
BENCHMARK(BenchArrayAt);

// std::get<>(array)是一个constexpr
// function，它会返回容器内元素的引用，并在编译期检查数组的索引是否正确
static void BenchArrayGet(benchmark::State& state) {
  std::array<int, len> arr;
  constexpr int i = 1;
  for (auto _ : state) {
    std::get<0>(arr) = MyPow(i);
    std::get<1>(arr) = MyPow(i + 1);
    std::get<2>(arr) = MyPow(i + 2);
    std::get<3>(arr) = MyPow(i + 3);
    std::get<4>(arr) = MyPow(i + 4);
    std::get<5>(arr) = MyPow(i + 5);
  }
}
BENCHMARK(BenchArrayGet);

BENCHMARK_MAIN();