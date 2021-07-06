#include "async/runtime/task_graph.h"

#include <thread>

#include "async/context/async_value.h"
#include "async/context/chain.h"
#include "async/context/host_context.h"
#include "async/support/latch.h"

namespace sss {

void TaskNode::AddDependency(TaskNode* node) {
  node->mSuccessories.push_back(this);
  mDependencies.push_back(node);
}

void TaskNode::AddSuccessor(TaskNode* node) {
  mSuccessories.push_back(node);
  node->mDependencies.push_back(this);
}

void TaskNode::operator()() { mComputeFunc(); }

void TaskGraph::BuildGraph() {
  for (size_t i = 0, e = mTaskNodes.size(); i < e; ++i) {
    mNodeIndexInfo[mTaskNodes[i].get()] = i;
    mNodeCountInfo[mTaskNodes[i].get()] = mTaskNodes[i]->GetNumDependencies();
  }
}

void TaskGraph::Destroy() {
  this->~TaskGraph();
  GetContext()->Deallocate<TaskGraph>(this);
}

TaskGraphExecutor::TaskGraphExecutor(TaskGraph* graph) : mGraph(graph) {
  const auto& nodeCountInfo = mGraph->mNodeCountInfo;
  for (const auto& data : nodeCountInfo) {
    mNodeCountInfo[data.first].store(data.second);
  }
  mGraph->AddRef();
  mTotalTaskCount.store(mGraph->mTaskNodes.size());
  mFinishChain = GetContext()->MakeUnconstructedAsyncValueRef<async::Chain>();
}

void TaskGraphExecutor::Execute(TaskGraphExecutor* executor) {
  executor->Execute();
  executor->DropRef();
}

void TaskGraphExecutor::Execute() {
  std::vector<unsigned> readyNodeIdx;
  ProcessStartNodeIndex(&readyNodeIdx);
  ProcessReadyNodeIndexs(&readyNodeIdx);
}

void TaskGraphExecutor::ProcessStartNodeIndex(std::vector<unsigned>* readyNodeIndex) {
  for (auto& node : mGraph->mTaskNodes) {
    // 这个node可以直接被执行
    if (node->GetNumDependencies() == 0) {
      readyNodeIndex->push_back(mGraph->mNodeIndexInfo[node.get()]);
    }
  }
}

void TaskGraphExecutor::ProcessReadyNodeIndex(unsigned nodeId, std::vector<unsigned>* readyNodeIdx) {
  TaskNode* node = mGraph->mTaskNodes[nodeId].get();
  (*node)();  // 执行完毕，修改readyNodeIndex
  int count = mTotalTaskCount.fetch_sub(1);
  for (auto* successor : node->mSuccessories) {
    if (mNodeCountInfo[successor].fetch_sub(1) == 1) {
      readyNodeIdx->push_back(mGraph->mNodeIndexInfo[successor]);
    }
  }
  if (count == 1) {
    mFinishChain->emplace<async::Chain>();
  }
}

void TaskGraphExecutor::ProcessReadyNodeIndexs(std::vector<unsigned>* readyNodeIndex) {
  // 完成队列非空
  while (!readyNodeIndex->empty()) {
    for (auto iter = std::next(readyNodeIndex->begin(), 1); iter != readyNodeIndex->end(); ++iter) {
      unsigned kernelId = *iter;
      AddRef();
      GetContext()->EnqueueWork([this, kernelId]() {
        std::vector<unsigned> readyNodeIdxs = {kernelId};
        this->ProcessReadyNodeIndexs(&readyNodeIdxs);
        this->DropRef();
      });
    }
    unsigned firstNodeId = readyNodeIndex->front();
    readyNodeIndex->clear();
    ProcessReadyNodeIndex(firstNodeId, readyNodeIndex);
  }
}

void TaskGraphExecutor::Await() {
  async::latch lat(1);
  std::vector<async::AsyncValue*> input;
  input.push_back(mFinishChain.get());
  GetContext()->RunWhenReady(absl::MakeConstSpan(input.data(), input.size()), [&lat]() { lat.count_down(1); });
  lat.wait();
}

void TaskGraphExecutor::Destroy() {
  this->~TaskGraphExecutor();
  GetContext()->Deallocate<TaskGraphExecutor>(this);
}

void RunTaskGraph(TaskGraph* graph, bool sync) {
  async::HostContext* context = graph->GetContext();
  TaskGraphExecutor* memory = context->Allocate<TaskGraphExecutor>();
  TaskGraphExecutor* executor = new (memory) TaskGraphExecutor(graph);
  executor->Execute();
  if (sync) executor->Await();
  executor->DropRef();
}

async::RCReference<TaskGraph> CreateTaskGraph(async::HostContext* context) {
  TaskGraph* memory = context->Allocate<TaskGraph>();
  TaskGraph* graphOri = new (memory) TaskGraph(context);
  async::RCReference<TaskGraph> graph = TakeRef(graphOri);
  return graph;
}

}  // namespace sss