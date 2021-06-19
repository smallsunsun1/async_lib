#include <condition_variable>
#include <mutex>
#include <vector>

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "async/concurrent/concurrent_work_queue.h"
#include "async/context/async_value.h"

namespace sss {
namespace async {

class SingleThreadedWorkQueue : public ConcurrentWorkQueue {
 public:
  SingleThreadedWorkQueue() {}
  std::string name() const override { return "single-threaded"; }
  void AddTask(TaskFunction work) override;
  absl::optional<TaskFunction> AddBlockingTask(TaskFunction work, bool allow_queuing) override;
  void Quiesce() override;
  void Await(absl::Span<const RCReference<AsyncValue>> values) override;
  int GetParallelismLevel() const override { return 1; }
  bool IsInWorkerThread() const final { return false; }

 private:
  mutable std::mutex mMu;
  std::condition_variable mCv;
  std::vector<TaskFunction> mWorkItems;
};

void SingleThreadedWorkQueue::AddTask(TaskFunction work) {
  {
    std::lock_guard<std::mutex> l(mMu);
    mWorkItems.push_back(std::move(work));
  }
  mCv.notify_all();
}
absl::optional<TaskFunction> SingleThreadedWorkQueue::AddBlockingTask(TaskFunction work, bool allow_queuing) {
  if (!allow_queuing) return {std::move(work)};
  {
    std::lock_guard<std::mutex> l(mMu);
    mWorkItems.push_back(std::move(work));
  }
  mCv.notify_all();
  return absl::nullopt;
}

void SingleThreadedWorkQueue::Quiesce() {
  std::vector<TaskFunction> local_work_items;
  while (true) {
    {
      std::lock_guard<std::mutex> l(mMu);
      if (mWorkItems.empty()) break;
      std::swap(local_work_items, mWorkItems);
    }
    for (auto& item : local_work_items) {
      item();
    }
    local_work_items.clear();
  }
}
void SingleThreadedWorkQueue::Await(absl::Span<const RCReference<AsyncValue>> values) {
  int values_remaining = values.size();
  for (auto& value : values) {
    value->AndThen([this, &values_remaining]() mutable {
      {
        std::lock_guard<std::mutex> l(mMu);
        --values_remaining;
      }
      mCv.notify_all();
    });
  }
  auto has_values = [this, &values_remaining]() mutable -> bool {
    std::lock_guard<std::mutex> l(mMu);
    return values_remaining != 0;
  };
  auto no_items_and_values_remaining = [this, &values_remaining]() -> bool { return values_remaining != 0 && mWorkItems.empty(); };
  std::vector<TaskFunction> local_work_items;
  int next_work_item_index = 0;
  while (has_values()) {
    if (next_work_item_index == local_work_items.size()) {
      local_work_items.clear();
      {
        std::unique_lock<std::mutex> l(mMu);
        while (no_items_and_values_remaining()) {
          mCv.wait(l);
        }
        if (values_remaining == 0) break;
        std::swap(local_work_items, mWorkItems);
      }
      next_work_item_index = 0;
    }
    local_work_items[next_work_item_index]();
    ++next_work_item_index;
  }
  if (next_work_item_index != local_work_items.size()) {
    std::lock_guard<std::mutex> l(mMu);
    mWorkItems.insert(mWorkItems.begin(), std::make_move_iterator(local_work_items.begin() + next_work_item_index), std::make_move_iterator(local_work_items.end()));
  }
}

std::unique_ptr<ConcurrentWorkQueue> CreateSingleThreadedWorkQueue() { return std::make_unique<SingleThreadedWorkQueue>(); }

}  // namespace async
}  // namespace sss
