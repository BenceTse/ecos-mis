/* 
* 版权声明: PKUSZ 216 
* 文件名称 : EventLoop.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: Reactor
* 历史记录: 无 
*/
/* 
 * 事件循环，即Reactor，一个线程最多一个Reactor 
 * 这是一个接口类 
 * 
 * Reactor是整个框架最核心的部分
 * 它创建轮询器（Poller），创建用于传递消息的管道，
 * 初始化各个部分，然后进入一个无限循环，
 * 每一次循环中调用轮询器的轮询函数（等待函数），
 * 等待事件发生，如果有事件发生，就依次调用套接字（或其他的文件描述符）
 * 的事件处理器处理各个事件，
 * 然后调用投递的回调函数；接着进入下一次循环。
 */
#pragma once

#include <logger/Logger.hpp>
#include <logger/AsynLogging.hpp>
#include <netbase/Poller.hpp>
#include <netbase/TimerQueue.hpp>
#include <base/MutexLockGuard.h>

#include <poll.h>
#include <sys/eventfd.h>

namespace netbase
{

class Channel;
class Poller;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
    // noncopyable
    EventLoop(const EventLoop&) = delete;
    EventLoop & operator=(const EventLoop&) = delete;

    typedef std::function<void()> Functor;
    // 事件循环，必须在同一个线程内创建Reactor对象和调用它的loop函数  
    void loop();
    // 跳出事件循环
    void quit();

    // 断言在指定线程中运行
    void assertInLoopThread()
    {
        if(!isInLoopThread())
            abortNotInLoopThread();
    }

    bool isInLoopThread() const 
    {
        return threadId_ == Concurrency::CurrentThread::tid();
    }

    bool hasChannel(Channel *channel) const {
        return poller_->hasChannel(channel);
    }

    // 静态成员方法，获取本线程中的eventloop对象指针
    static EventLoop* getEventLoopOfCurrentThread();

    // 新增channel，则需要在poller中报备
    void updateChannel(Channel* channel);
    // 删除channel
    void removeChannel(Channel* channel);

    // 可跨线程调用该函数，但无论在什么线程，传入的仿函子都必然在eventloop所在的线程中执行
    void runInLoop(const Functor& functor);
    // void runInLoop(Functor& functor);
    // 如果本就在eventloop线程中，则直接插队运行
    void queueInLoop(const Functor& functor);
    // 如果确定functor不再使用，则可以使用移动语义
    // 函数有大量数据时，考虑到数据会被拷贝到functor中，所以尽量使用移动语义
    void queueInLoop(Functor&& functor);

    bool looping() { return looping_; }

    // the interfaces of timerqueue
    // the measurement of delay is second
    // 一个eventloop绑定一个定时器
    // 在指定时间点执行某项任务
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    // 在某段时间后执行某项任务
    TimerId runAfter(double delay, const TimerCallback& cb);
    // 每隔一定时间重复执行某项任务
    TimerId runEvery(double interval, const TimerCallback& cb);

    // It's OK when cancel twice
    // 取消一个计时器
    void cancel(const TimerId& timerId) { timerqueue_->cancel(timerId);}

private:
    // 如果创建Reactor的线程和运行Reactor的线程不同就退出进程 
    void abortNotInLoopThread();

    // 唤醒沉睡中eventloop
    void wakeup();
    // 处理读读事件
    void handleRead();
    // 执行在沉睡过程中填充的任务
    void doPendingFunctors();
    int createEventfd();
    // 事件处理器列表
    typedef std::vector<Channel*> ChannelList;
    // 是否正在循环中 
    bool looping_; /* atomic */
    // 是否退出循环
    bool quit_; /* atomic */
    // 线程id  
    const pid_t threadId_;
    // 轮询器  
    std::unique_ptr<Poller> poller_;
    // 已激活的事件处理器队列 
    ChannelList activeChannels_;
    // 是否正在调用投递的回调函数 
    bool callingPendingFunctors_;
    // 用于唤醒的描述符（将Reactor从等待中唤醒，一般是由于调用轮询器的等待函数而造成的阻塞）
    int wakeupfd_;
    // 唤醒事件的处理器 
    std::unique_ptr<Channel> wakeupChannel_;
    // 投递的回调函数列表
    std::vector<Functor> functors_;
    MutexLock mutex_;
    // 定时器队列
    std::unique_ptr<TimerQueue> timerqueue_;
    
    long long iteration_;
};

}
