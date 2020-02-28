/* 
* 版权声明: PKUSZ 216 
* 文件名称 : CountDownLatch.hpp
* 创建日期: 2018/03/04 
* 文件描述: 倒数计数器,给定一个计数，等待的线程一直等到该计数变为0才能继续运行
* 历史记录: 无 
*/
#pragma once
#include <mutex>
#include <condition_variable>

class CountDownLatch {
public:
    CountDownLatch(size_t ct) : ct_(ct) {}
    // 等待，直到count变为0
    void wait();
    // 等待，直到count变为0或达到预定时间
    void wait_for(size_t nSeconds);
    // 计数减1
    void countDown();
    // 获取计数  
    int getCount();
private:
    std::mutex mutex_;// 锁 
    std::condition_variable cv_;// 线程是否可以继续运行的条件变量
    int ct_;// 计数
};
