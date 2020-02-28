/* 
* 版权声明: PKUSZ 216 
* 文件名称 : EventLoopThread.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 用于执行Reactor（EventLoop）循环的线程 
* 历史记录: 无 
*/
#include <netbase/EventLoopThread.hpp>
#include <netbase/EventLoop.hpp>
#include <assert.h>

#include <unistd.h>

namespace netbase
{
/* 
 * 构造函数 
 */ 
EventLoopThread::EventLoopThread(const std::string& name)
    : EventLoopThread(name, nullptr)
{
}

EventLoopThread::EventLoopThread(const std::string& name, const ThreadInitCallback& cb)
    : loop_(nullptr),
      mutex_(),
      condition_(mutex_),
      thread_(name, std::bind(&EventLoopThread::threadFunc, this)),
      callback_(cb)
{
}
/* 
 * 析构函数 
 */
EventLoopThread::~EventLoopThread()
{
    assert(thread_.started());
    if(loop_ != nullptr)
    {
        // 退出循环
        loop_->quit();
        // 等待线程结束
        thread_.join();
    }
}

// 开始循环
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    // 启动线程 
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        // 等待线程启动完毕
        condition_.wait( [this] {
            return (this->loop_ != nullptr);
        });
    }
    return loop_;
}

// 线程函数：用于执行EVentLoop的循环 
void EventLoopThread::threadFunc()
{
    EventLoop eventloop;
    // 如果有初始化函数，就先调用初始化函数 
    if(callback_)
    {
        callback_(&eventloop);
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &eventloop;
        // 通知startLoop线程已经启动完毕
        condition_.notify();
    }
    // 事件循环
    loop_->loop();
    loop_ = nullptr;
}

}
