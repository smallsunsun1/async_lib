#ifndef INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_TASK_GRAPH_
#define INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_TASK_GRAPH_

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "async/support/ref_count.h"

namespace ficus {

namespace async {
class AsyncValue;
class HostContext;
}  // namespace async

class TaskNode;
class TaskGraph;
class TaskGraphExecutor;

class TaskNode {
 public:
  TaskNode() = default;
  TaskNode(std::function<void()> computeFunc) : mComputeFunc(std::move(computeFunc)) {}
  void AddDependency(TaskNode* node);
  void AddSuccessor(TaskNode* node);
  std::vector<TaskNode*>& GetDependencies() { return mDependencies; }
  const std::vector<TaskNode*>& GetDependencies() const { return mDependencies; }
  std::vector<TaskNode*>& GetSuccessor() { return mSuccessories; }
  const std::vector<TaskNode*>& GetSuccessor() const { return mSuccessories; }
  unsigned GetNumSuccessor() { return mSuccessories.size(); }
  unsigned GetNumDependencies() { return mDependencies.size(); }
  void operator()();

 private:
  friend class TaskGraph;
  friend class TaskGraphExecutor;
  std::function<void()> mComputeFunc;  // 实际计算函数
  std::vector<TaskNode*> mDependencies;
  std::vector<TaskNode*> mSuccessories;
};

class TaskGraph : public async::ReferenceCounted<TaskGraph> {
 public:
  TaskGraph() = default;
  TaskGraph(async::HostContext* context) : mpContext(context) {}
  template <typename GeneralFunc>
  TaskNode* emplace_back(GeneralFunc&& f) {
    TaskNode* node = new TaskNode(std::forward<GeneralFunc>(f));
    mTaskNodes.emplace_back(node);
    return node;
  }
  void Destroy();
  async::HostContext* GetContext() { return mpContext; }
  void BuildGraph();

 private:
  friend class TaskNode;
  friend class TaskGraphExecutor;
  async::HostContext* mpContext;
  std::unordered_map<TaskNode*, unsigned> mNodeIndexInfo;  // 记录每个Node对应的Index
  std::unordered_map<TaskNode*, int> mNodeCountInfo;       // 记录每个Node有多少Dependency
  std::vector<std::unique_ptr<TaskNode>> mTaskNodes;
};

class TaskGraphExecutor : public async::ReferenceCounted<TaskGraphExecutor> {
 public:
  TaskGraphExecutor(TaskGraph* graph);
  ~TaskGraphExecutor() {
    if (mGraph) mGraph->DropRef();
  }
  static void Execute(TaskGraphExecutor* executor);
  void Execute();
  void ProcessReadyNodeIndex(unsigned nodeId, std::vector<unsigned>* readyNodeIdx);
  void ProcessReadyNodeIndexs(std::vector<unsigned>* readyNodeIndex);
  void ProcessStartNodeIndex(std::vector<unsigned>* readyNodeIndex);
  void Await();
  void Destroy();
  async::HostContext* GetContext() { return mGraph->mpContext; }

 private:
  std::unordered_map<TaskNode*, std::atomic<int>> mNodeCountInfo;
  std::atomic<int> mTotalTaskCount;  // 总共有多少任务
  async::RCReference<async::AsyncValue> mFinishChain;
  TaskGraph* mGraph;
};

}  // namespace ficus

#endif /* INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_TASK_GRAPH_ */
