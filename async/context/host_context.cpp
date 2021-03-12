#include "host_context.h"

#include <iostream>
#include <mutex>
#include <thread>

#include "../concurrent/concurrent_work_queue.h"
#include "../support/string_util.h"
#include "chain.h"
#include "function.h"
#include "host_allocator.h"
#include "location.h"

namespace ficus {
namespace async {

void LocationHandler::VtableAnchor() {}
std::atomic<int> HostContext::num_shared_context_types_{0};
static std::atomic<int> next_host_context_index{0};
HostContext* HostContext::all_host_contexts_[HostContextPtr::kDummyIndex];

HostContext::HostContext(
    std::function<void(const DecodedDiagnostic&)> diag_handler,
    std::unique_ptr<HostAllocator> allocator,
    std::unique_ptr<ConcurrentWorkQueue> work_queue)
    : mDiagHandler(std::move(diag_handler)),
      mAllocator(std::move(allocator)),
      mWorkQueue(std::move(work_queue)),
      mSharedCtxMgr(std::make_unique<SharedContextManager>(this)),
      mInstancePtr(next_host_context_index.fetch_add(1)) {
  assert(instance_index() < HostContextPtr::kDummyIndex &&
         "Created too many HostContext instances");
  all_host_contexts_[instance_index()] = this;
  mReadyChain = MakeAvailableAsyncValueRef<Chain>();
}

HostContext::~HostContext() {
  // Wait for the completion of all async tasks managed by this host context.
  Quiesce();
  // We need to free the ready chain AsyncValue first, as the destructor of the
  // AsyncValue calls the HostContext to free its memory.
  mReadyChain.reset();
  all_host_contexts_[instance_index()] = nullptr;
}

void Function::VtableAnchor() {}

// Construct an empty IndirectAsyncValue, not forwarding to anything.
RCReference<IndirectAsyncValue> HostContext::MakeIndirectAsyncValue() {
  return TakeRef(Construct<IndirectAsyncValue>(mInstancePtr));
}

//===----------------------------------------------------------------------===//
// Error Reporting
//===----------------------------------------------------------------------===//

// Emit an error for a specified decoded diagnostic, which gets funneled
// through a location handler.
void HostContext::EmitError(const DecodedDiagnostic& diagnostic) {
  // Emit the message to the global handler, guaranteeing that it will be seen
  // by the handler registered with the HostContext.
  mDiagHandler(diagnostic);
}

// Create a ConcreteAsyncValue in error state for a specified decoded
// diagnostic.
RCReference<ErrorAsyncValue> HostContext::MakeErrorAsyncValueRef(
    DecodedDiagnostic&& diagnostic) {
  // Create an AsyncValue for this error condition.
  auto* error_value =
      Construct<ErrorAsyncValue>(mInstancePtr, std::move(diagnostic));

  return TakeRef(error_value);
}

// Create a ConcreteAsyncValue in error state for a specified error message.
RCReference<ErrorAsyncValue> HostContext::MakeErrorAsyncValueRef(
    absl::string_view message) {
  return MakeErrorAsyncValueRef(DecodedDiagnostic(message));
}

void HostContext::CancelExecution(absl::string_view msg) {
  // Create an AsyncValue in error state for cancel.
  auto* error_value = MakeErrorAsyncValueRef(msg).release();

  AsyncValue* expected_value = nullptr;
  // Use memory_order_release for the success case so that error_value is
  // visible to other threads when they load with memory_order_acquire. For the
  // failure case, we do not care about expected_value, so we can use
  // memory_order_relaxed.
  if (!mCancelValue.compare_exchange_strong(expected_value, error_value,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
    error_value->DropRef();
  }
}

void HostContext::Restart() {
  // Use memory_order_acq_rel so that previous writes on this thread are visible
  // to other threads and previous writes from other threads (e.g. the return
  // 'value') are visible to this thread.
  auto value = mCancelValue.exchange(nullptr, std::memory_order_acq_rel);
  if (value) {
    value->DropRef();
  }
}

//===----------------------------------------------------------------------===//
// Memory Management
//===----------------------------------------------------------------------===//

// Allocate the specified number of bytes at the specified alignment.
void* HostContext::AllocateBytes(size_t size, size_t alignment) {
  return mAllocator->AllocateBytes(size, alignment);
}

// Deallocate the specified pointer, that had the specified size.
void HostContext::DeallocateBytes(void* ptr, size_t size) {
  mAllocator->DeallocateBytes(ptr, size);
}

//===----------------------------------------------------------------------===//
// Concurrency
//===----------------------------------------------------------------------===//

void HostContext::Quiesce() { mWorkQueue->Quiesce(); }

void HostContext::Await(absl::Span<const RCReference<AsyncValue>> values) {
  mWorkQueue->Await(values);
}

// Add some work to the workqueue managed by this CPU device.
void HostContext::EnqueueWork(unique_function<void()> work) {
  mWorkQueue->AddTask(TaskFunction(std::move(work)));
}

// Add some work to the workqueue managed by this CPU device.
bool HostContext::EnqueueBlockingWork(unique_function<void()> work) {
  absl::optional<TaskFunction> task = mWorkQueue->AddBlockingTask(
      TaskFunction(std::move(work)), /*allow_queuing=*/true);
  return !task.has_value();
}

int HostContext::GetNumWorkerThreads() const {
  return mWorkQueue->GetParallelismLevel();
}

// Run the specified function when the specified set of AsyncValue's are all
// resolved.  This is a set-version of "AndThen".
void HostContext::RunWhenReady(absl::Span<AsyncValue* const> values,
                               unique_function<void()> callee) {
  // Perform a quick scan of the arguments.  If they are all available, or if
  // any is already an error, then we can run the callee synchronously.
  absl::InlinedVector<AsyncValue*, 4> unavailable_values;
  for (auto i : values) {
    if (!i->IsAvailable()) unavailable_values.push_back(i);
  }

  // If we can synchronously call 'callee', then do it and we're done.
  if (unavailable_values.empty()) return callee();

  // If there is exactly one unavailable value, then we can just AndThen it.
  if (unavailable_values.size() == 1) {
    unavailable_values[0]->AndThen(
        [callee = std::move(callee)]() mutable { callee(); });
    return;
  }

  struct CounterAndCallee {
    std::atomic<size_t> counter;
    unique_function<void()> callee;
  };

  // Otherwise, we have multiple unavailable values.  Put a counter on the heap
  // and have each unavailable value decrement and test it.
  auto* data =
      new CounterAndCallee{{unavailable_values.size()}, std::move(callee)};

  for (auto* val : unavailable_values) {
    val->AndThen([data]() {
      // Decrement the counter unless we're the last to be here.
      if (data->counter.fetch_sub(1) != 1) return;

      // If we are the last one, then run the callee and free the data.
      data->callee();
      delete data;
    });
  }
}

void HostContext::RunWhenReady(absl::Span<const RCReference<AsyncValue>> values,
                               unique_function<void()> callee) {
  absl::InlinedVector<AsyncValue*, 8> values_ptr;
  values_ptr.reserve(values.size());
  for (auto& data : values) {
    values_ptr.push_back(data.get());
  }
  RunWhenReady(absl::MakeConstSpan(values_ptr.data(), values_ptr.size()),
               std::move(callee));
}

//===----------------------------------------------------------------------===//
// SharedContext management
//===----------------------------------------------------------------------===//
//
class HostContext::SharedContextManager {
 public:
  explicit SharedContextManager(HostContext* host) : host_{host} {}
  // Returns the shared context instance with the given shared_context_id.
  // Create one if the requested shared context instance does not exist yet.
  SharedContext& GetOrCreateSharedContext(int shared_context_id,
                                          SharedContextFactory factory) {
    assert(shared_context_id < shared_context_instances_.size() &&
           "The requested SharedContext ID exceeds the maximum allowed");

    auto& item = shared_context_instances_[shared_context_id];

    std::call_once(item.second, [&] {
      assert(!item.first);
      item.first = factory(host_);
    });

    return *item.first;
  }

 private:
  HostContext* const host_;
  // We allow up to 256 ShareContext instances.
  std::array<std::pair<std::unique_ptr<SharedContext>, std::once_flag>, 256>
      shared_context_instances_{};
};

SharedContext& HostContext::GetOrCreateSharedContext(
    int shared_context_id, SharedContextFactory factory) {
  return mSharedCtxMgr->GetOrCreateSharedContext(shared_context_id, factory);
}

bool HostContext::IsInWorkerThread() const {
  return mWorkQueue->IsInWorkerThread();
}

SharedContext::~SharedContext() {}

std::unique_ptr<HostContext> CreateSimpleHostContext() {
  return std::make_unique<HostContext>(
      [](const DecodedDiagnostic& in) { std::cout << in.message << std::endl; },
      CreateMallocAllocator(),
      CreateMultiThreadedWorkQueue(std::thread::hardware_concurrency(),
                                   2 * std::thread::hardware_concurrency()));
}

std::unique_ptr<HostContext> CreateCustomHostContext(int numNonBlockThreads,
                                                     int numBlockThreads) {
  return std::make_unique<HostContext>(
      [](const DecodedDiagnostic& in) { std::cout << in.message << std::endl; },
      CreateMallocAllocator(),
      CreateMultiThreadedWorkQueue(numNonBlockThreads, numBlockThreads));
}

}  // namespace async
}  // namespace ficus
