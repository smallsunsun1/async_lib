#include "async_value.h"

#include <functional>

#include "host_context.h"
#include "task_function.h"

namespace ficus {
namespace async {
class NotifierListNode {
 public:
  explicit NotifierListNode(unique_function<void()> notification)
      : mNext(nullptr), mNotification(std::move(notification)) {}

 private:
  friend class AsyncValue;
  NotifierListNode* mNext;
  unique_function<void()> mNotification;
};
uint16_t AsyncValue::CreateTypeInfoAndReturnTypeIdImpl(Destructor destructor) {
  TypeInfo typeInfo{destructor};
  size_t typeId = GetTypeInfoTableSingleton()->emplace_back(typeInfo) + 1;
  // Detect overflow.
  assert(typeId < std::numeric_limits<uint16_t>::max() &&
         "Too many different AsyncValue types.");
  return typeId;
}
AsyncValue::TypeInfoTable* AsyncValue::GetTypeInfoTableSingleton() {
  const int kInitialCapacity = 64;
  static auto* typeInfoTable = new TypeInfoTable(kInitialCapacity);
  return typeInfoTable;
}
std::atomic<ssize_t> AsyncValue::total_allocated_async_values_;
const AsyncValue::TypeInfo& AsyncValue::GetTypeInfo() const {
  TypeInfoTable* typeInfoTable = AsyncValue::GetTypeInfoTableSingleton();
  assert(mTypeId != 0);

  // TODO(sanjoy): Once ConcurentVector supports it, we should check that
  // mTypeId - 1 is within range.
  return (*typeInfoTable)[mTypeId - 1];
}
void AsyncValue::Destroy() {
  if (kind() == Kind::kIndirect) {
    // Depending on what the benchmarks say, it might make sense to remove this
    // explicit check and instead make ~IndirectAsyncValue go through the
    // GetTypeInfo().destructor case below.
    static_cast<IndirectAsyncValue*>(this)->~IndirectAsyncValue();
    GetHostContext()->DeallocateBytes(this, sizeof(IndirectAsyncValue));
    return;
  }

  auto size = GetTypeInfo().destructor(this, /*destroys_object=*/true);
  GetHostContext()->DeallocateBytes(this, size);
}
// This is called when the value is set into the ConcreteAsyncValue buffer, or
// when the IndirectAsyncValue is forwarded to an available AsyncValue, and we
// need to change our state and clear out the notifications. The current state
// must be unavailable (i.e. kUnconstructed or kConstructed).
void AsyncValue::NotifyAvailable(State availableState) {
  assert((kind() == Kind::kConcrete || kind() == Kind::kIndirect) &&
         "Should only be used by ConcreteAsyncValue or IndirectAsyncValue");

  assert(availableState == State::kConcrete || availableState == State::kError);

  // Mark the value as available, ensuring that new queries for the state see
  // the value that got filled in.
  auto oldValue = mWaitersAndState.exchange(
      WaitersAndState(nullptr, availableState), std::memory_order_acq_rel);
  assert(oldValue.getInt() == State::kUnconstructed ||
         oldValue.getInt() == State::kConstructed);

  RunWaiters(oldValue.getPointer());
}

void AsyncValue::RunWaiters(NotifierListNode* list) {
  HostContext* host = GetHostContext();
  while (list) {
    auto* node = list;
    // TODO(chky): pass state into mNotification so that waiters do not need to
    // check atomic state again.
    node->mNotification();
    list = node->mNext;
    host->Destruct(node);
  }
}

// If the value is available or becomes available, this calls the closure
// immediately. Otherwise, the add closure to the waiter list where it will be
// called when the value becomes available.
void AsyncValue::EnqueueWaiter(unique_function<void()>&& waiter,
                               WaitersAndState oldValue) {
  // Create the node for our waiter.
  auto* node = GetHostContext()->Construct<NotifierListNode>(std::move(waiter));
  auto old_state = oldValue.getInt();

  // Swap the next link in. oldValue.getInt() must be unavailable when
  // evaluating the loop condition. The acquire barrier on the compare_exchange
  // ensures that prior changes to waiter list are visible here as we may call
  // RunWaiter() on it. The release barrier ensures that prior changes to *node
  // appear to happen before it's added to the list.
  node->mNext = oldValue.getPointer();
  auto new_value = WaitersAndState(node, old_state);
  while (!mWaitersAndState.compare_exchange_weak(oldValue, new_value,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire)) {
    // While swapping in our waiter, the value could have become available.  If
    // so, just run the waiter.
    if (oldValue.getInt() == State::kConcrete ||
        oldValue.getInt() == State::kError) {
      assert(oldValue.getPointer() == nullptr);
      node->mNext = nullptr;
      RunWaiters(node);
      return;
    }
    // Update the waiter list in new_value.
    node->mNext = oldValue.getPointer();
  }

  // compare_exchange_weak succeeds. The oldValue must be in either
  // kUnconstructed or kConstructed state.
  assert(oldValue.getInt() == State::kUnconstructed ||
         oldValue.getInt() == State::kConstructed);
}

void AsyncValue::SetError(DecodedDiagnostic diag_in) {
  auto s = state();
  assert(s == State::kUnconstructed || s == State::kConstructed);

  if (kind() == Kind::kConcrete) {
    if (s == State::kConstructed) {
      // ~AsyncValue erases mTypeId and makes a few assertion on real
      // destruction, but this AsyncValue is still alive.
      GetTypeInfo().destructor(this, /*destroys_object=*/false);
    }
    char* thisPtr = reinterpret_cast<char*>(this);
    auto& error = *reinterpret_cast<DecodedDiagnostic**>(
        thisPtr + AsyncValue::kDataOrErrorOffset);
    error = new DecodedDiagnostic(std::move(diag_in));
    NotifyAvailable(State::kError);
  } else {
    assert(kind() == Kind::kIndirect);
    auto error_av = mHostContext->MakeErrorAsyncValueRef(std::move(diag_in));
    static_cast<IndirectAsyncValue*>(this)->ForwardTo(std::move(error_av));
  }
}

void AsyncValue::SetErrorLocationIfUnset(DecodedLocation location) {
  auto& diag = const_cast<DecodedDiagnostic&>(GetError());
  if (!diag.location) diag.location = std::move(location);
}

// Mark this IndirectAsyncValue as forwarding to the specified value.  This
// gives the IndirectAsyncValue a +1 reference.
void IndirectAsyncValue::ForwardTo(RCReference<AsyncValue> value) {
  assert(IsUnavailable());

  auto s = value->state();
  if (s == State::kConcrete || s == State::kError) {
    assert(!mValue && "IndirectAsyncValue::ForwardTo is called more than once");
    auto* concreteValue = value.release();
    if (IndirectAsyncValue::classof(concreteValue)) {
      auto* indirectValue = static_cast<IndirectAsyncValue*>(concreteValue);
      concreteValue = indirectValue->mValue;
      assert(concreteValue != nullptr);
      assert(concreteValue->kind() == Kind::kConcrete);
      concreteValue->AddRef();
      indirectValue->DropRef();
    }
    mValue = concreteValue;
    mTypeId = concreteValue->mTypeId;
    NotifyAvailable(s);
  } else {
    // Copy value here because the evaluation order of
    // value->AndThen(std::move(value)) is not defined prior to C++17.
    AsyncValue* value2 = value.get();
    value2->AndThen(
        [this2 = FormRef(this), value2 = std::move(value)]() mutable {
          this2->ForwardTo(std::move(value2));
        });
  }
}

}  // namespace async
}  // namespace ficus