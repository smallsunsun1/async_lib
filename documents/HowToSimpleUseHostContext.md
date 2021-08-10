# 创建HostContext  
直接创建最常用的HostContext，该函数会创建等同于硬件core数量的无锁线程池和对应core数量x2的普通线程池  
std::unique_ptr<HostContext>  CreateSimpleHostContext();  

用户直接制定无锁和带锁线程池的线程数  
std::unique_ptr<HostContext>   CreateCustomHostContext(int numNonBlockThreads,
                                                     int numBlockThreads);

                                                
# 提交任务到线程池
带返回值的快速计算任务  
template <typename F, typename R, std::enable_if_t<!std::is_void<R>(), int>>  
AsyncValueRef<R> HostContext::EnqueueWork(F &&work);                     
带返回值的IO相关任务                   
template <typename F, typename R, std::enable_if_t<!std::is_void<R>(), int>>
AsyncValueRef<R> HostContext::EnqueueBlockingWork(F &&work);
不带返回值的快速计算任务  
void EnqueueWork(unique_function<void()> work);  
不带返回值的IO相关任务  
bool EnqueueBlockingWork(unique_function<void()> work);

# 如果通过Await阻塞使得一个AsyncValue的状态变为Ready  
1. 阻塞主线程，等待线程池中所有任务完成(需要注意的是该函数不应该在worker线程内被调用)  
    void Quiesce();  
2. 阻塞主线程，等待特定的一些AsyncValue相关的任务完成  
    void Await(absl::Span<const RCReference<AsyncValue>> values);  

# 如何创建AsyncValue(optional，一般不太会需要用到)  
构建一个实际数据T被创建，但为默认初始状态的AsyncValue  
template <typename T, typename... Args>  
  AsyncValueRef<T> MakeConstructedAsyncValueRef(Args &&...args);  

构建一个实际数据T还未被创建的AsyncValue  
template <typename T>  
  AsyncValueRef<T> MakeUnconstructedAsyncValueRef();  

构建一个包含数据数据T，且已经被赋值的AsyncValue  
template <typename T, typename... Args>  
  AsyncValueRef<T> MakeAvailableAsyncValueRef(Args &&...args);  

构建一个IndirectAsyncValue, 在异步计算中作为placeholder用于转发存储还未被计算出的AsyncValue.  
  RCReference<IndirectAsyncValue> MakeIndirectAsyncValue();  