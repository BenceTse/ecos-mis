/* 
* 版权声明: PKUSZ 216 
* 文件名称 : CountDownLatch.cc
* 创建日期: 2018/03/04 
* 文件描述: 倒数计数器,给定一个计数，等待的线程一直等到该计数变为0才能继续运行
* 历史记录: 无 
*/
#include "CountDownLatch.hpp"
// 等待，直到计数变为0  
void CountDownLatch::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !ct_; });
}

void CountDownLatch::wait_for(size_t nSeconds) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::seconds(nSeconds), [this] { return !ct_; });
}

// 让计数减1 
void CountDownLatch::countDown() {
    std::lock_guard<std::mutex> lock(mutex_);
    --ct_;
    // 计数变为0就通知所有其他等待的线程
    if(!ct_) cv_.notify_all();
}
// 获取计数  
int CountDownLatch::getCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return ct_;
}
