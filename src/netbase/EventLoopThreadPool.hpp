/* 
* 版权声明: PKUSZ 216 
* 文件名称 : EventLoopThreadPool.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 事件循环线程池,这个类适用于一个进程中存在多个Reactor实例的情况 
* 历史记录: 无 
*/

#pragma once
#include <netbase/Callbacks.hpp>
#include <netbase/EventLoopThread.hpp>

#include <cstddef>
#include <vector>
#include <string>
#include <mutex>

namespace netbase
{

class EventLoopThread;
class EventLoop;

class EventLoopThreadPool
{
public:
    // EventLoopThreadPool(size_t nLoop, const std::string& name, const ThreadInitCallback& cb);
    // EventLoopThreadPool(size_t nLoop, const std::string& name, const ThreadInitCallback& cb);
    EventLoopThreadPool(EventLoop* baseLoop_, const std::string& name);
    // ~EventLoopThreadPool();

    void setThreadNum(size_t num) { nloops_ = num; }
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    // 设置线程池的线程数量
    void start();

    // thread-safe
    // 获取下一个EventLoop对象
    EventLoop* getNextLoop();

    EventLoop* getBaseLoop() {
        return baseLoop_;
    }

    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;
private:
    //主要的EventLoop对象
    EventLoop* baseLoop_;
    // 线程池的名字
    std::string name_;
    int next_;
    size_t nloops_;
    bool started_;
    // 线程列表 
    std::vector<std::unique_ptr<EventLoopThread> > loopThreads_;
    // EventLoop对象列表
    std::vector<EventLoop*> loops_;
    ThreadInitCallback threadInitCallback_;
    std::mutex mutex_;
};

}
