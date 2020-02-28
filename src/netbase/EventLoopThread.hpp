/* 
* 版权声明: PKUSZ 216 
* 文件名称 : EventLoopThread.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 用于执行Reactor（EventLoop）循环的线程 
* 历史记录: 无 
*/
/* 
 * 
 * 这个类专门创建一个线程用于执行Reactor的事件循环
 * 当然这只是一个辅助类，没有说一定要使用它，可以根据自己的情况进行选择
 * 你也可以不创建线程去执行事件循环，而在主线程中执行事件循环，一切根据自己的需要
 * 
 * **NOTES**：强烈建议使用该类。原因在于本类中的EventLoop是栈对象，
 * 这样就不用考虑EventLoop的析构问题。它的生命期永远和EventLoopThread一样长。
 * 只要调用quit函数即可退出事件轮询，不用（也不能）delete EventLoop
 * 
 * 相反如果直接使用EventLoop，绝大部分情况下要使用堆对象，这样调用了
 * quit()函数后，不能确保能立刻delete它
 */
#pragma once
#include <functional>
#include <base/Thread.hpp>
#include <base/MutexLockGuard.h>
#include <base/Condition.h>
#include <netbase/Callbacks.hpp>

namespace netbase
{

class EventLoop;

class EventLoopThread
{
public:
    // 线程初始化回调函数
    EventLoopThread(const std::string& name);
    EventLoopThread(const std::string& name, const ThreadInitCallback& cb);
    ~EventLoopThread();
    // 启动循环
    EventLoop* startLoop();

    void join() {thread_.join();}

private:
    // 线程函数用于执行EventLoop的循环  
    void threadFunc();
    // 对应的Reactor
    EventLoop* loop_;
    MutexLock mutex_;
    // 条件变量，用于exiting_变化的通知
    Condition condition_;
    // 执行Reactor循环的线程 
    Concurrency::Thread thread_;
    // 线程初始化回调函数
    ThreadInitCallback callback_;
    // bool exiting_;

};


}
