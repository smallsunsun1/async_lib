#include <iostream>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../support/status.h"
#include "gtest/gtest.h"

using namespace ficus;
using namespace async;

struct TestData {
  TestData() = default;
  TestData(TestData&& data) {
    std::cout << "Move Constructor Called!" << std::endl;
    mData = std::move(data.mData);
  }
  TestData(const TestData& data) {
    std::cout << "Copy Constructor Called!" << std::endl;
    mData = data.mData;
  }
  TestData(const std::vector<int>& vec) : mData(vec) {}
  std::vector<int> mData;
};

TEST(STDFUNCTION, V1) {
  TestData data;
  StatusAnd<TestData> s0 = make_status<TestData>(1, data);
  StatusAnd<TestData> s = make_status<TestData>(0, std::move(data));
  StatusAnd<TestData> s2 = make_status<TestData>(2);
  TestData data2;
  std::tuple<TestData> t = std::make_tuple(std::move(data2));
  EXPECT_EQ(s2.errorCode, 2);
  EXPECT_EQ(s2.mData.mData.size(), 0);
  s2.emplace(std::vector<int>(2));
  EXPECT_EQ(s2.mData.mData.size(), 2);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  (void)RUN_ALL_TESTS();
  return 0;
}
