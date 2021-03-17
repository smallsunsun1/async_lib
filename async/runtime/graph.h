#ifndef INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_GRAPH_
#define INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_GRAPH_

#include <string>
#include <unordered_map>
#include <vector>

#include "async/context/async_value.h"
#include "async/context/native_function.h"
#include "async/support/ref_count.h"
#include "async_kernel.h"

namespace sss {
namespace async {
class AsyncValue;
class Function;
class HostContext;
class CommonAsyncKernelFrame;
}  // namespace async

class GraphExecutor;
class AsyncGraph;

class AsyncNode {
 public:
  AsyncNode(std::vector<std::string> inputNames, std::vector<std::string> outputNames, AsyncKernelFn fn, const std::string& fname = "", bool isStrictFunc = true)
      : mInputNames(std::move(inputNames)), mOutputNames(std::move(outputNames)), mFunc(std::move(fn)), mIsStrictFunc(isStrictFunc), mFuncName(fname) {}
  void operator()(async::CommonAsyncKernelFrame* kernelFrame) { mFunc(kernelFrame); }
  void AddInputs(const std::string& name) { mInputNames.push_back(name); }
  void AddOutputs(const std::string& name) { mOutputNames.push_back(name); }
  unsigned GetNumResults() { return mOutputNames.size(); }
  unsigned GetNumInputs() { return mInputNames.size(); }
  absl::Span<const std::string> GetInputNames() { return mInputNames; }
  const std::string& GetInputNameAt(int index) const { return mInputNames[index]; }
  absl::Span<const std::string> GetOutputNames() { return mOutputNames; }
  const std::string GetOutputNameAt(int index) const { return mOutputNames[index]; }

 private:
  friend class GraphExecutor;
  friend class AsyncGraph;
  AsyncKernelFn mFunc;
  bool mIsStrictFunc = true;  // 实际用于计算的函数
  const std::string mFuncName;
  std::vector<std::string> mInputNames;   // 用于指示当前Node输入变量的Name
  std::vector<std::string> mOutputNames;  // 用于指示当前Node输出变量的Name
};

class AsyncGraph : public async::ReferenceCounted<AsyncGraph> {
 public:
  AsyncGraph(async::HostContext* context) : mpContext(context) {}
  AsyncGraph() = delete;
  AsyncGraph(const AsyncGraph&) = delete;
  AsyncGraph& operator=(const AsyncGraph&) = delete;
  AsyncGraph(AsyncGraph&&) = delete;
  ~AsyncGraph();
  void Destroy();
  void BuildGraph();
  void Dump(const std::string& filename) const;
  void Load(const std::string& filename);
  async::HostContext* GetContext() { return mpContext; }
  AsyncNode* emplace_back(std::vector<std::string> inputNames, std::vector<std::string> outputNames, AsyncKernelFn fn, const std::string& name = "", bool isStrictFunc = true);
  unsigned GetNumOutputs() const;
  std::vector<std::string> GetOutputNames() const;

 private:
  friend class GraphExecutor;
  async::HostContext* mpContext;
  std::unordered_map<std::string, unsigned> mNameAndAsyncIdPair;    // key表示unique_name，对应GraphExec调用中的AsyncValueInfo
  std::unordered_map<AsyncNode*, KernelResInfo> mUsedByKernlTable;  // 用于指示当前AsyncNode的result会被哪些后续AsyncNode使用
  FunctionInfo mFunctionInfo;                                       // AsyncValue(use-count),
                                                                    // kernel-info(指示多少Arguments还未Ready)
  std::vector<AsyncNode*> mAsyncNodes;                              // 这里需要保证在GraphExecutor被释放的时候这些Node资源也会被释放
  bool mIsConstructed = false;
};

// 这个类用于将整个Function作为一个Graph来执行，构建Node的时候需要注意的是要先构建最初的Arguments
// Node 后续需要添加从文件Load以及Save到文件的相关代码 调用static
// GraphExecutor::Execute(Args&&)后，代码在ThreadPool执行，用户不需要管理后续资源，GraphExecutor
// 会负责所有Arguments的内存释放和对自身的内存释放
class GraphExecutor : public async::ReferenceCounted<GraphExecutor> {
 public:
  GraphExecutor(AsyncGraph* inGraph) : graph(inGraph) {
    assert(graph->mIsConstructed && "Graph Must Be Constructed");
    graph->AddRef();
    mFunctionInfo.mAsyncValueInfos.reserve(graph->mFunctionInfo.mAsyncValueInfos.size());
    for (size_t i = 0, e = graph->mFunctionInfo.mAsyncValueInfos.size(); i != e; ++i) {
      mFunctionInfo.mAsyncValueInfos.emplace_back(graph->mFunctionInfo.mAsyncValueInfos[i].mUserCount);
    }
    mFunctionInfo.mKernelInfos.reserve(graph->mFunctionInfo.mKernelInfos.size());
    for (size_t i = 0, e = graph->mFunctionInfo.mKernelInfos.size(); i != e; ++i) {
      mFunctionInfo.mKernelInfos.emplace_back(graph->mFunctionInfo.mKernelInfos[i].mArgumentsNotReady.load(std::memory_order_relaxed));
    }
    // DebugFn();
  }
  ~GraphExecutor();
  // 释放当前的graph
  void Destroy();
  // 开始进行计算的函数
  void Execute();
  // 辅助函数，用于对相应Kernel的argument的readyCount -
  // 1，如果刚好使kernel所有Arguments变为Ready，将其加入到执行队列
  void DecreaseReadyCountAndPush(absl::Span<const unsigned> users, std::vector<unsigned>* readyKernelIdxs);
  // 辅助函数，用于对当前result进行更新，同时更新后续的AsyncNode*的readyArgumentsCount
  void ProcessUsedByAndSetAsyncValueInfo(absl::Span<const unsigned> users, std::vector<unsigned>* readyKernelIdxs, async::RCReference<async::AsyncValue> result, AsyncValueInfo* info);
  // 类似于上面的ProcessUsedByAndSetAsyncValueInfo，但是用于Fake KernelFn
  void ProcessPseudoKernelUsedBys(absl::Span<const unsigned> users, std::vector<unsigned>* readyKernelIdx, async::AsyncValue* result);
  // 作为第一个Node执行的Fake函数，因为第一个Node没有输入Arguments
  void ProcessArgumentsAsPseudoKernel(std::vector<unsigned>* readyKernelIdxs);
  // 计算已经Ready的Kernel函数
  void ProcessReadyKernel(unsigned kernelId, async::CommonAsyncKernelFrame* kernelFrame, std::vector<unsigned>* readyKernelIdx);
  // 在ProcessArgumentsAsPseudoKernel后调用，计算所有后续的结果
  void ProcessReadyKernels(std::vector<unsigned>* readyKernelIdxs);
  // 获取当前AsyncNode的第resultNumber个输出会被哪些Kernel所使用
  absl::Span<const unsigned> GetNextUsedBys(AsyncNode* resInfo, int resultNumber);
  // Static用于外部调用的函数
  static void Execute(GraphExecutor* executor, absl::Span<async::AsyncValue* const> arguments, absl::Span<async::RCReference<async::AsyncValue>> results);
  // 对输入ready的valueInfo进行初始化
  void InitializeArgumentRegisters(absl::Span<async::AsyncValue* const> arguments, absl::Span<AsyncValueInfo> asyncValueInfos);
  // 对结果所在的valueInfo进行初始化
  void InitializeResultRegisters(std::vector<unsigned>* resultReg);
  // 用于打印Graph中间状态结果，主要用于Debug
  void DebugFn();
  // 获取每个AsyncValue的状态
  absl::Span<AsyncValueInfo> GetAsyncValueInfo();
  // 获取每个Kernel的状态信息
  absl::Span<KernelInfo> GetKernelInfo();
  async::HostContext* GetContext() { return graph->GetContext(); }

 public:
  friend class async::ReferenceCounted<GraphExecutor>;
  AsyncGraph* graph;
  FunctionInfo mFunctionInfo;  // AsyncValue(use-count),
                               // kernel-info(指示多少Arguments还未Ready)
};

void RunAsyncGraph(AsyncGraph* graph, std::vector<async::RCReference<async::AsyncValue>>& arguments, std::vector<async::RCReference<async::AsyncValue>>& results, bool sync = true);

}  // namespace sss

#endif /* INFERENCE_MEDICAL_BIOIMAGE_BRAIN_COMMON_GRAPH_GRAPH_ */
