#ifndef ASYNC_COROUTINE_COROUTINE_NOTIFIER_
#define ASYNC_COROUTINE_COROUTINE_NOTIFIER_

#include <mutex>
#include <condition_variable>

class CoroutineNotifier {
public:
    CoroutineNotifier(bool isSet): mIsSet(isSet) {}
    CoroutineNotifier() = default;
    ~CoroutineNotifier() = default;
    void Set() {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mIsSet) {
            mIsSet = true;
            mCv.notify_one();
        }
    }
    void Wait() {
        std::unique_lock<std::mutex> lock(mMutex);
        mCv.wait(lock, [this](){return mIsSet == true;});
        mIsSet = true;
    }
private:
    bool mIsSet = false;
    std::mutex mMutex;
    std::condition_variable mCv;
};

#endif /* ASYNC_COROUTINE_COROUTINE_NOTIFIER_ */
