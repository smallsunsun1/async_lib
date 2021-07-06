#include "graph.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>

#include "async/context/async_value.h"
#include "async/context/kernel_frame.h"
#include "async/context/native_function.h"
#include "async/runtime/register.h"

namespace sss {

using namespace async;

// 一些Helper函数
AsyncValue* GetAsyncValuePtr(const AsyncValueInfo& info) { return info.mValue.load(std::memory_order_acquire); }

AsyncValue* GetOrCreateAsyncValuePtr(AsyncValueInfo* info, HostContext* ctx) {
  AsyncValue* value = info->mValue.load(std::memory_order_acquire);
  if (value) return value;  // Value已经被创建，直接Return
  auto* indirectValue = ctx->MakeIndirectAsyncValue().release();
  AsyncValue* exisiting = nullptr;
  // 令indirectValue的引用计算变为userCount+1
  indirectValue->AddRef(info->mUserCount);
  if (!info->mValue.compare_exchange_strong(exisiting, indirectValue, std::memory_order_release, std::memory_order_acquire)) {
    // 如果mValue已经被赋值过
    indirectValue->DropRef(info->mUserCount + 1);
    return exisiting;
  } else {
    return indirectValue;
  }
}

void SetAsyncValuePtr(AsyncValueInfo* info, RCReference<AsyncValue> result) {
  AsyncValue* exisiting = nullptr;
  auto newValue = result.release();  // newValue 拥有1 reference
  // 如果当前结果后续不再有使用者，那么该结果为返回值
  unsigned refAddValue = 0;
  if (info->mUserCount != 0) {
    refAddValue = info->mUserCount - 1;
  }
  newValue->AddRef(refAddValue);  // newValue变为useCount
  if (!info->mValue.compare_exchange_strong(exisiting, newValue, std::memory_order_release, std::memory_order_acquire)) {
    // 结果已经存在，那么必定是IndirectAsyncValue，直接cast
    auto indirectValue = static_cast<IndirectAsyncValue*>(exisiting);
    newValue->DropRef(refAddValue);
    indirectValue->ForwardTo(TakeRef(newValue));
  }
}

void ProcessNameAndRefCount(const std::string& name, int* startId, std::map<std::string, int>& countMap, std::unordered_map<std::string, unsigned>& indexMap) {
  auto iter = countMap.find(name);
  if (iter != countMap.end()) {
    iter->second += 1;
  } else {
    countMap[name] = 1;
  }
  auto iter2 = indexMap.find(name);
  if (iter2 == indexMap.end()) {
    indexMap[name] = *startId;
    (*startId)++;
  }
}

AsyncGraph::~AsyncGraph() {
  for (auto node : mAsyncNodes) {
    if (node) {
      GetContext()->Destruct(node);
    }
  }
}

void AsyncGraph::Destroy() {
  auto ctx = GetContext();
  this->~AsyncGraph();
  ctx->Deallocate<AsyncGraph>(this);
}

RCReference<AsyncGraph> AsyncGraph::SubGraph(const std::vector<std::string>& outputNames) {
  std::unordered_set<const AsyncNode*> traveledNodes;              // record which node has beed traveled
  std::unordered_map<std::string, const AsyncNode*> nameNodeMap;   // record name and asyncnode map
  RCReference<AsyncGraph> graph = CreateAsyncGraph(GetContext());  // create return graph

  for (const auto* node : mAsyncNodes) {
    for (const std::string& outName : node->mOutputNames) {
      nameNodeMap[outName] = node;
    }
  }
  std::queue<std::string> queuedNames;
  for (const std::string& name : outputNames) {
    queuedNames.push(name);
  }
  while (!queuedNames.empty()) {
    const std::string& name = queuedNames.front();
    const AsyncNode* node = nameNodeMap[name];
    if (traveledNodes.find(node) == traveledNodes.end()) {
      traveledNodes.emplace(node);
      graph->emplace(node->mInputNames, node->mOutputNames, node->mFunc, node->mFuncName);
      for (const std::string& inputName : node->mInputNames) {
        queuedNames.push(inputName);
      }
    }
    queuedNames.pop();
  }
  return graph;
}

void AsyncGraph::BuildGraph() {
  std::map<std::string, int> argCountMap;                            // 记录每个变量名以及其作为Arguments对应的refCount
  std::map<std::string, int> resCountMap;                            // 记录每个变量名以及其作为Results对应的refCount
  std::unordered_map<std::string, unsigned> indexAsyncValueMap;      // 记录每个变量命及其对应的Asyncvalue在整个队列中的Id
  std::unordered_map<std::string, std::set<unsigned>> usedCountmap;  // 记录每个变量命会被哪些AsyncNode所使用
  std::unordered_map<std::string, AsyncNode*> indexAsyncNodeMap;     // 记录每个变量命及对应产生该变量result的AsyncNode
  std::unordered_map<std::string, unsigned> indexAsyncNodeIdMap;     // 记录每个变量命及对应产生该变量result的AsyncNode在整个队列中的Id
  int startId = 0;
  int asyncNodeStartId = 0;
  for (int i = 0, numNodes = mAsyncNodes.size(); i != numNodes; ++i) {
    AsyncNode* curNode = mAsyncNodes[i];
    // 计算统计每个变量的RefCount
    for (int j = 0, e = curNode->GetNumInputs(); j != e; ++j) {
      ProcessNameAndRefCount(curNode->GetInputNameAt(j), &startId, argCountMap, indexAsyncValueMap);
      usedCountmap[curNode->GetInputNameAt(j)].insert(asyncNodeStartId);
    }
    for (int j = 0, e = curNode->GetNumResults(); j != e; ++j) {
      ProcessNameAndRefCount(curNode->GetOutputNameAt(j), &startId, resCountMap, indexAsyncValueMap);
      indexAsyncNodeMap[curNode->GetOutputNameAt(j)] = curNode;
      indexAsyncNodeIdMap[curNode->GetOutputNameAt(j)] = asyncNodeStartId;
    }
    asyncNodeStartId++;
  }
  mFunctionInfo.mKernelInfos.resize(mAsyncNodes.size());
  // 初始化mUsedByKernlTable
  for (size_t i = 0, numNodes = mAsyncNodes.size(); i != numNodes; ++i) {
    AsyncNode* curNode = mAsyncNodes[i];
    // 计算统计每个变量的后续的User kernel对应的Index
    KernelResInfo kernelResInfo;
    // 初始化当前kernel的Result会被哪些KernelId所使用的vector容器
    kernelResInfo.mResultTable.resize(curNode->GetNumResults());
    for (uint32_t j = 0, e = curNode->GetNumResults(); j < e; ++j) {
      const auto& container = usedCountmap[curNode->GetOutputNameAt(j)];
      kernelResInfo.mResultTable[j] = std::vector<unsigned>(container.begin(), container.end());
      mFunctionInfo.mKernelInfos[indexAsyncNodeIdMap[curNode->GetOutputNameAt(j)]].mArgumentsNotReady.store(indexAsyncNodeMap[curNode->GetOutputNameAt(j)]->GetNumInputs());  // 初始化KernelInfo结果
    }
    mUsedByKernlTable[curNode] = std::move(kernelResInfo);
  }
  mFunctionInfo.mAsyncValueInfos.resize(indexAsyncValueMap.size());
  for (auto& value : indexAsyncValueMap) {
    int totalCount = argCountMap[value.first] + resCountMap[value.first];  // 获取全部的引用计数
    mFunctionInfo.mAsyncValueInfos[value.second].mUserCount = totalCount - 1;
  }
  mNameAndAsyncIdPair = std::move(indexAsyncValueMap);

  // 获取输出结果的名称
  for (const auto& iter : mNameAndAsyncIdPair) {
    if (mFunctionInfo.mAsyncValueInfos[iter.second].mUserCount == 0) {
      mOutputNames.push_back(iter.first);
    }
  }
  mIsConstructed = true;
}

AsyncNode* AsyncGraph::emplace(const std::vector<std::string>& inputNames, const std::vector<std::string>& outputNames, const AsyncKernelFn& fn, const std::string& name) {
  AsyncNode* node = GetContext()->Construct<AsyncNode>(std::move(inputNames), std::move(outputNames), std::move(fn), name);
  mAsyncNodes.push_back(node);
  return node;
}

void AsyncGraph::Dump(const std::string& filename) const {
  std::ofstream outFile(filename);
  for (size_t i = 0, numNodes = mAsyncNodes.size(); i < numNodes; ++i) {
    for (size_t j = 0, numInputs = mAsyncNodes[i]->GetNumInputs(); j < numInputs; ++j) {
      outFile << "input: " << mAsyncNodes[i]->GetInputNameAt(j) << "\n";
    }
    for (size_t j = 0, numOutputs = mAsyncNodes[i]->GetNumResults(); j < numOutputs; ++j) {
      outFile << "output: " << mAsyncNodes[i]->GetOutputNameAt(j) << "\n";
    }
    outFile << "kernel name: " << mAsyncNodes[i]->mFuncName << "\n";
    outFile << "\n";
  }
  outFile.close();
}

void AsyncGraph::Load(const std::string& filename) {
  std::ifstream inFile(filename);
  std::string graphLineInfo;
  std::vector<std::string> inputNames;
  std::vector<std::string> outputNames;
  AsyncKernelFn kernelFn;
  std::string kernelName;
  while (std::getline(inFile, graphLineInfo)) {
    if (graphLineInfo != "") {
      std::vector<std::string> res = StrSplit(graphLineInfo, ": ");
      assert(res.size() == 2 && "must need 2 info to restore node");
      if (res[0] == "input") {
        inputNames.push_back(res[1]);
      } else if (res[0] == "output") {
        outputNames.push_back(res[1]);
      } else if (res[0] == "kernel name") {
        std::optional<AsyncKernelFn> kernelFunction = GET_KERNEL_FN(res[1]);
        kernelName = res[1];
        if (!kernelFunction.has_value()) {
          std::cerr << "can't find kernel name: " << res[1] << "\n";
          assert(false);
        }
        kernelFn = kernelFunction.value();
      }
    } else {
      (void)emplace(inputNames, outputNames, kernelFn, kernelName);
      inputNames.clear();
      outputNames.clear();
      kernelName.clear();
    }
  }
}

void AsyncGraph::Reset() {
  mNameAndAsyncIdPair.clear();
  mUsedByKernlTable.clear();
  mFunctionInfo.mAsyncValueInfos.clear();
  mFunctionInfo.mKernelInfos.clear();
  for (auto node : mAsyncNodes) {
    if (node) {
      GetContext()->Destruct(node);
    }
  }
  mAsyncNodes.clear();
  mOutputNames.clear();
  mIsConstructed = false;
}

unsigned AsyncGraph::GetNumOutputs() const { return static_cast<unsigned>(mOutputNames.size()); }

std::vector<std::string> AsyncGraph::GetOutputNames() const { return mOutputNames; }

void GraphExecutor::InitializeArgumentRegisters(absl::Span<AsyncValue* const> arguments, absl::Span<AsyncValueInfo> asyncValueInfos) {
  AsyncNode* startNode = nullptr;
  for (size_t i = 0, e = graph->mAsyncNodes.size(); i < e; ++i) {
    if (graph->mAsyncNodes[i]->GetNumInputs() == 0) {
      startNode = graph->mAsyncNodes[i];
      break;
    }
  }
  assert(startNode && "startNode can't be nullptr");
  assert(arguments.size() >= startNode->GetNumResults() && "Arguments Size Must Larger Than StartNode Result");
  for (size_t i = 0; i < startNode->GetNumResults(); ++i) {
    const std::string& name = startNode->GetOutputNameAt(i);
    unsigned idx = graph->mNameAndAsyncIdPair[name];
    AsyncValue* value = arguments[i];
    value->AddRef(asyncValueInfos[idx].mUserCount);
    asyncValueInfos[idx].mValue = value;
  }
}

void GraphExecutor::InitializeResultRegisters(std::vector<unsigned>* resultReg) {
  for (const auto& iter : graph->mNameAndAsyncIdPair) {
    // 如果某个AsyncValue不会被后面的结果使用到，那么该AsyncValue是需要被return的
    if (mFunctionInfo.mAsyncValueInfos[iter.second].mUserCount == 0) {
      resultReg->push_back(iter.second);
    }
  }
}

// GraphExecutor相关函数
void GraphExecutor::Destroy() {
  auto ctx = GetContext();
  this->~GraphExecutor();
  ctx->Deallocate<GraphExecutor>(this);
}
GraphExecutor::~GraphExecutor() { graph->DropRef(); }

void GraphExecutor::Execute() {
  std::vector<unsigned> readyKernelIdx;
  // 最初的kernel id是0，这是初始Node，没有输入Arguments，因此需要特殊处理
  ProcessArgumentsAsPseudoKernel(&readyKernelIdx);
  ProcessReadyKernels(&readyKernelIdx);
}

void GraphExecutor::DecreaseReadyCountAndPush(absl::Span<const unsigned> users, std::vector<unsigned>* readyKernelIdxs) {
  // KernelInfo记录了每个Kernel还有多少的Arguments还未Ready
  auto kernelInfo = GetKernelInfo();
  for (unsigned userId : users) {
    // 如果全部Arguments已经Ready，在readyKernel的队列中加入相应结果
    if (kernelInfo[userId].mArgumentsNotReady.fetch_sub(1) == 1) {
      readyKernelIdxs->push_back(userId);
    }
  }
}

void GraphExecutor::ProcessUsedByAndSetAsyncValueInfo(absl::Span<const unsigned> users, std::vector<unsigned>* readyKernelIdxs, async::RCReference<async::AsyncValue> result,
                                                      AsyncValueInfo* resultInfo) {
  // 如果当前的result不被任何user所用到，那么直接将其复制到resultInfo
  if (users.empty()) {
    SetAsyncValuePtr(resultInfo, std::move(result));
    return;
  }
  auto state = result->state();
  // 如果当前Value的状态已经可以被获取，直接对后续的所有users中kernel的ready
  // arguments -1
  if (state.IsAvailable()) {
    SetAsyncValuePtr(resultInfo, std::move(result));
    DecreaseReadyCountAndPush(users, readyKernelIdxs);
    return;
  }
  // 接下去处理result尚未ready的情况
  // 保证当前GraphExecutor存活
  AddRef();
  auto* resultPtr = result.get();
  resultPtr->AndThen([this, users, resultInfo, result = std::move(result)]() mutable {
    std::vector<unsigned> readyKernelIdxs;
    SetAsyncValuePtr(resultInfo, std::move(result));
    this->DecreaseReadyCountAndPush(users, &readyKernelIdxs);
    this->ProcessReadyKernels(&readyKernelIdxs);  // 执行已经Ready的KernelIndex
    this->DropRef();                              // 释放当前Executor的引用
  });
}

void GraphExecutor::ProcessArgumentsAsPseudoKernel(std::vector<unsigned>* readyKernelIdxs) {
  assert(readyKernelIdxs->empty() && "ReadyKernelIdxs Must Be Empty");
  absl::Span<AsyncValueInfo> asyncValueInofs = GetAsyncValueInfo();
  // 这是初始化Arguments数据，初始化的Node的Index为0，我们需要保证第一个Node的输入为空，输出不为空
  AsyncNode* asyncNode = nullptr;
  for (size_t i = 0, e = graph->mAsyncNodes.size(); i < e; ++i) {
    if (graph->mAsyncNodes[i]->GetNumInputs() == 0) {
      asyncNode = graph->mAsyncNodes[i];
      break;
    }
  }
  assert(asyncNode && "async node can't be nullptr");
  assert(asyncNode->GetNumInputs() == 0);
  assert(asyncNode->GetNumResults() != 0);

  // 获取当前AsyncFn的第一个输出会被多少人使用
  auto usedBy = GetNextUsedBys(asyncNode, 0);
  DecreaseReadyCountAndPush(usedBy, readyKernelIdxs);
  for (uint32_t resultNumber = 1, e = asyncNode->GetNumResults(); resultNumber < e; ++resultNumber) {
    auto& valueInfo = asyncValueInofs[graph->mNameAndAsyncIdPair[asyncNode->GetOutputNameAt(resultNumber)]];
    if (valueInfo.mUserCount == 0) continue;
    AsyncValue* result = GetAsyncValuePtr(valueInfo);
    assert(result && "Argument AsyncValue Is Not Set");
    auto usedBy = GetNextUsedBys(asyncNode, resultNumber);
    // 处理这些结果的使用者
    ProcessPseudoKernelUsedBys(usedBy, readyKernelIdxs, result);
  }
}

void GraphExecutor::ProcessPseudoKernelUsedBys(absl::Span<const unsigned> users, std::vector<unsigned>* readyKernelIdx, async::AsyncValue* result) {
  if (users.empty()) return;
  auto state = result->state();
  if (state.IsAvailable()) {
    DecreaseReadyCountAndPush(users, readyKernelIdx);
    return;
  }
  AddRef();
  result->AndThen([this, users]() {
    std::vector<unsigned> readyKernelIdx;
    this->DecreaseReadyCountAndPush(users, &readyKernelIdx);
    this->ProcessReadyKernels(&readyKernelIdx);
    this->DropRef();
  });
}

void GraphExecutor::ProcessReadyKernels(std::vector<unsigned>* readyKernelIdx) {
  CommonAsyncKernelFrame kernelFrame(GetContext());
  while (!readyKernelIdx->empty()) {
    // 逐个计算已经Ready的相应Kernel
    for (auto iter = std::next(readyKernelIdx->begin(), 1); iter != readyKernelIdx->end(); ++iter) {
      unsigned kernelId = *iter;
      AddRef();
      GetContext()->EnqueueWork([this, kernelId]() {
        std::vector<unsigned> readyKernelIdxs = {kernelId};
        this->ProcessReadyKernels(&readyKernelIdxs);
        this->DropRef();
      });
    }
    // 对于第一个ready的kernel，在当前函数被计算
    unsigned firstKernelId = readyKernelIdx->front();
    readyKernelIdx->clear();  // 清空当前readyKernelIdx
    kernelFrame.Reset();      // 初始化清空当前KernelFrame
    ProcessReadyKernel(firstKernelId, &kernelFrame, readyKernelIdx);
  }
}

void GraphExecutor::ProcessReadyKernel(unsigned kernelId, async::CommonAsyncKernelFrame* kernelFrame, std::vector<unsigned>* readyKernelIdx) {
  absl::Span<AsyncValueInfo> asyncValueInfoArr = GetAsyncValueInfo();
  AsyncValue* errorArguments = nullptr;
  // 获取实际对应要被运行的AsyncNode
  AsyncNode* node = graph->mAsyncNodes[kernelId];
  std::vector<unsigned> argumentsIdx;
  argumentsIdx.reserve(node->GetNumInputs());
  for (uint32_t i = 0, e = node->GetNumInputs(); i < e; ++i) {
    argumentsIdx.push_back(graph->mNameAndAsyncIdPair[node->GetInputNameAt(i)]);
  }
  std::vector<unsigned> resultsIndex;
  resultsIndex.reserve(node->GetNumResults());
  for (uint32_t i = 0, e = node->GetNumResults(); i < e; ++i) {
    resultsIndex.push_back(graph->mNameAndAsyncIdPair[node->GetOutputNameAt(i)]);
  }
  for (auto regIdx : argumentsIdx) {
    AsyncValueInfo& asyncValueInfo = asyncValueInfoArr[regIdx];
    AsyncValue* value = GetOrCreateAsyncValuePtr(&asyncValueInfo, GetContext());
    kernelFrame->AddArg(value);
    if (value->IsError()) errorArguments = value;
  }
  kernelFrame->SetNumResults(node->GetNumResults());
  if (errorArguments == nullptr) {
    (*node)(kernelFrame);
  } else {
    for (size_t i = 0, e = kernelFrame->GetNumResults(); i != e; ++i) {
      kernelFrame->SetResultAt(i, FormRef(errorArguments));
    }
  }
  // 现在Kernel已经执行完毕，丢弃相应的Ref引用
  for (auto* arg : kernelFrame->GetArguments()) {
    arg->DropRef();
  }
  for (uint32_t resultNumber = 0, e = node->GetNumResults(); resultNumber < e; ++resultNumber) {
    auto& resultInfo = asyncValueInfoArr[resultsIndex[resultNumber]];
    // 确保存储的AsyncValue还未被赋值
    assert(GetAsyncValuePtr(resultInfo) == nullptr || GetAsyncValuePtr(resultInfo)->IsUnresolvedIndirect());
    AsyncValue* result = kernelFrame->GetResultAt(resultNumber);
    assert(result && "Kernel did not set result AsyncValue");
    auto usedBy = GetNextUsedBys(node, resultNumber);
    ProcessUsedByAndSetAsyncValueInfo(usedBy, readyKernelIdx, TakeRef(result), &resultInfo);
  }
}

void GraphExecutor::Execute(GraphExecutor* executor, absl::Span<async::AsyncValue* const> arguments, absl::Span<async::RCReference<async::AsyncValue>> results) {
  std::vector<unsigned> resultRegs;
  absl::Span<AsyncValueInfo> asyncValueInfoArr = executor->GetAsyncValueInfo();
  executor->InitializeArgumentRegisters(arguments, asyncValueInfoArr);
  executor->InitializeResultRegisters(&resultRegs);  // 预初始化结果Idx
  executor->Execute();
  for (size_t i = 0, e = results.size(); i != e; ++i) {
    assert(!results[i] && "result AsyncValue is not nullptr");
    AsyncValueInfo& resultReg = asyncValueInfoArr[resultRegs[i]];
    AsyncValue* value = GetOrCreateAsyncValuePtr(&resultReg,
                                                 executor->GetContext());  // 这里可能是IndirectAsyncValue或者ConcreteAsyncValue
    results[i] = TakeRef(value);
  }
  executor->DropRef();
}

void GraphExecutor::DebugFn() {
  std::cout << "Check Unsed By Kernel\n";
  for (auto value : graph->mUsedByKernlTable) {
    std::cout << "first: " << value.first << "\n";
    for (size_t i = 0, e = value.second.mResultTable.size(); i < e; ++i) {
      std::cout << "index: " << i << "\n";
      for (size_t j = 0, e2 = value.second.mResultTable[i].size(); j < e2; ++j) {
        std::cout << value.second.mResultTable[i][j] << "\n";
      }
      std::cout << " \n";
    }
  }
  std::cout << "Check Function Info\n";
  std::cout << "Check AsyncValue Infos\n";
  for (size_t i = 0, e = mFunctionInfo.mAsyncValueInfos.size(); i < e; ++i) {
    std::cout << "user count: " << mFunctionInfo.mAsyncValueInfos[i].mUserCount << " value: " << mFunctionInfo.mAsyncValueInfos[i].mValue << "\n";
  }
  std::cout << "Check Kernel Infos\n";
  for (size_t i = 0, e = mFunctionInfo.mKernelInfos.size(); i < e; ++i) {
    std::cout << "KERNEL: " << mFunctionInfo.mKernelInfos[i].mArgumentsNotReady << "\n";
  }
  std::cout << "Check Final Nodes\n";
  for (size_t i = 0, e = graph->mAsyncNodes.size(); i < e; ++i) {
    std::cout << "Nodes: " << graph->mAsyncNodes[i] << "\n";
  }
}

absl::Span<const unsigned> GraphExecutor::GetNextUsedBys(AsyncNode* resInfo, int resultNumber) {
  auto& kernelResInfo = graph->mUsedByKernlTable[resInfo].mResultTable[resultNumber];
  return absl::MakeConstSpan(kernelResInfo.data(), kernelResInfo.size());
}

absl::Span<AsyncValueInfo> GraphExecutor::GetAsyncValueInfo() { return absl::MakeSpan(mFunctionInfo.mAsyncValueInfos.data(), mFunctionInfo.mAsyncValueInfos.size()); }

absl::Span<KernelInfo> GraphExecutor::GetKernelInfo() { return absl::MakeSpan(mFunctionInfo.mKernelInfos.data(), mFunctionInfo.mKernelInfos.size()); }

void RunAsyncGraph(AsyncGraph* graph, std::vector<RCReference<AsyncValue>>& arguments, std::vector<RCReference<AsyncValue>>& results, bool sync) {
  auto* runContext = graph->GetContext();
  auto* execPtr = runContext->Allocate<GraphExecutor>();
  GraphExecutor* exec = new (execPtr) GraphExecutor(graph);
  std::vector<AsyncValue*> argumentsPtr;
  argumentsPtr.reserve(arguments.size());
  for (auto& elem : arguments) {
    argumentsPtr.push_back(elem.get());
  }
  results.resize(graph->GetNumOutputs());
  GraphExecutor::Execute(exec, absl::MakeConstSpan(argumentsPtr.data(), argumentsPtr.size()), absl::MakeSpan(results.data(), results.size()));
  if (sync) runContext->Await(results);
}

async::RCReference<AsyncGraph> CreateAsyncGraph(async::HostContext* context) {
  AsyncGraph* memory = context->Allocate<AsyncGraph>();
  AsyncGraph* graphOri = new (memory) AsyncGraph(context);
  RCReference<AsyncGraph> graph = TakeRef(graphOri);
  return graph;
}

}  // namespace sss