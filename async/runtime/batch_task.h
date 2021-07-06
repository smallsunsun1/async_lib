#ifndef INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_BATCH_TASK_
#define INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_BATCH_TASK_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

#include "async/context/host_context.h"
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "third_party/eigen3/unsupported/Eigen/CXX11/ThreadPool"

namespace sss {
namespace async {

class BatchTask {
 public:
  virtual ~BatchTask() = default;
  virtual size_t Size() const = 0;
};

// 这里的TenosrBatchTask目前作为一个通用的Task模块，因此Size=1
template <typename T>
class TensorBatchTask : public BatchTask {
 public:
  TensorBatchTask() = default;
  template <typename... Args,
            std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
  TensorBatchTask(Args &&...args) : mData(std::forward<Args>(args)...) {}
  TensorBatchTask(std::unique_ptr<T> data) : mData(std::move(data)) {}
  TensorBatchTask &operator=(const TensorBatchTask &) = delete;
  TensorBatchTask(const TensorBatchTask &) = delete;
  ~TensorBatchTask() = default;
  size_t Size() const override { return 1; }

 private:
  std::unique_ptr<T> mData;
};
template <typename TaskType>
class Batch {
 public:
  Batch(const Batch &) = delete;
  Batch &operator=(const Batch &) = delete;
  Batch() = default;
  void AddTask(std::unique_ptr<TaskType> task) {
    std::lock_guard<std::mutex> lock(mMu);
    mTasks.push_back(std::move(task));
  }
  std::unique_ptr<TaskType> RemoveTask() {
    std::lock_guard<std::mutex> lock(mMu);
    std::unique_ptr<TaskType> res = std::move(mTasks.back());
    mTasks.pop_back();
    return res;
  }
  int NumTasks() const {
    std::lock_guard<std::mutex> lock(mMu);
    return mTasks.size();
  }
  bool Empty() const {
    std::lock_guard<std::mutex> lock(mMu);
    return mTasks.empty();
  }
  const TaskType &Task(int i) const {
    std::lock_guard<std::mutex> lock(mMu);
    return *mTasks[i];
  }
  TaskType *MutableTask(int i) {
    std::lock_guard<std::mutex> lock(mMu);
    return mTasks[i].get();
  }
  size_t Size() const {
    std::lock_guard<std::mutex> lock(mMu);
    return mSize;
  }
  bool IsClosed() const { return mClosed.load(); }
  void WaitUntilClosed() { mNotifier.Wait(); }
  void Close() {
    mNotifier.Notify();
    mClosed.store(true);
  }

 private:
  mutable std::mutex mMu;
  std::vector<std::unique_ptr<TaskType>> mTasks;
  size_t mSize;
  // 下面的变量用于指示当前batch是否已经close
  Eigen::Notification mNotifier;
  std::atomic<bool> mClosed;
};

template <typename TaskType>
class BatchScheduler {
 public:
  virtual ~BatchScheduler() = default;
  virtual bool Schedule(std::unique_ptr<TaskType> task) = 0;
  virtual size_t NumEnqueuedTasks() const = 0;
  virtual size_t SchedulingCapacity() const = 0;
  virtual size_t MaxTaskSize() const = 0;
};

template <typename TaskType>
class StreamBatchScheduler : public BatchScheduler<TaskType> {
 public:
  template <typename F>
  StreamBatchScheduler(F &&f, int maxBatchSize, int numBatchesInProcess,
                       int maxTaskSize, HostContext *ctx)
      : mProcessBatchCallback(std::forward<F>(f)),
        mMaxBatchSize(maxBatchSize),
        mNumBatchedInProgress(numBatchesInProcess),
        mMaxTaskSize(maxTaskSize),
        mCtx(ctx) {}
  ~StreamBatchScheduler() {
    std::lock_guard<std::mutex> lock(mMu);
    if (mOpenBatch != nullptr) {
      mOpenBatch->Close();
      mOpenBatch = nullptr;
      --mOpenBatchNum;
    }
  }
  size_t SchedulingCapacity() const override {}
  size_t NumEnqueuedTasks() const override {}
  size_t MaxTaskSize() const override { return mMaxTaskSize; }
  bool Schedule(std::unique_ptr<TaskType> task) override {
    if (task->Size() > mMaxBatchSize) return false;
    {
      std::lock_guard<std::mutex> lock(mMu);
      if (mOpenBatch == nullptr || !TaskFitsInBatch(task.get(), mOpenBatch)) {
        StartNewBatch();
      }
      mOpenBatch->AddTask(std::move(task));
      if (mOpenBatch->Size() == mMaxBatchSize) {
        StartNewBatch();
      }
    }
    return true;
  }
  bool TaskFitsInBatch(const TaskType *task,
                       const Batch<TaskType *> batch) const {
    return batch.Size() + task->Size() <= mMaxBatchSize;
  }

 private:
  void StartNewBatch() {
    if (mOpenBatch != nullptr) {
      mOpenBatch->Close();
      mOpenBatch = nullptr;
    }
    Batch<TaskType> *newOpenBatch = new Batch<TaskType>;
    ++mNumBatchedInProgress;
    mCtx->EnqueueBlockingWork([this, newOpenBatch]() {
      this->mProcessBatchCallback(
          std::unique_ptr<Batch<TaskType>>(newOpenBatch));
      {
        std::lock_guard<std::mutex> lock(this->mMu);
        --this->mNumBatchedInProgress;
      }
    });
    mOpenBatch = newOpenBatch;
    ++mOpenBatchNum;
  }
  // 用来对Batch数据进行处理的Callback，如把Batch数据凑一起，然后用某个函数infer
  std::function<void(std::unique_ptr<Batch<TaskType>>)> mProcessBatchCallback;
  mutable std::mutex mMu;
  int mMaxBatchSize;
  int mOpenBatchNum = 0;
  int mNumBatchedInProgress = 0;
  int mMaxTaskSize = 0;
  Batch<TaskType> *mOpenBatch = nullptr;
  HostContext *mCtx;
};

}  // namespace async
}  // namespace sss

#endif /* INFERENCE_MEDICAL_COMMON_CPP_ASYNC_RUNTIME_BATCH_TASK_ */
