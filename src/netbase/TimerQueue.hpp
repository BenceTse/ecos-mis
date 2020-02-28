/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TimerQueue.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 定时器队列 
* 历史记录: 无 
*/
#pragma once
#include <vector>
#include <set>

#include <netbase/Channel.hpp>
#include <netbase/Callbacks.hpp>

namespace netbase
{

class EventLoop;
class Timer;
class TimerId;
class Timestamp;

class TimerQueue
{
public:
    // TimerList的条目
    typedef std::pair<Timestamp, Timer*> Entry;
    // 平衡二叉树维护所有timer
    typedef std::set<Entry> TimerList;

    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // void addTimer(const TimerCallback& cb, Timestamp when, double interval);
    // 往timerlist中加入timer
    TimerId addTimer(const TimerCallback&& cb, Timestamp when, double interval);

    // 取消指定的timer
    void cancel(const TimerId& timerId);

    // 当timerfd被唤起的时候
    void handleRead();

    // 不可拷贝
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;

private:

    // 获取所有的逾期timer
    std::vector<Entry> getExpired(Timestamp now);
    // int createTimerfd();

    // for addTimer
    // 确保addTimer在单一的eventLoop线程中执行
    void addTimerInLoop(Timer* timer);
    bool insert(Timer* timer);

    // 重置定时器
    void reset(std::vector<Entry>& expired, Timestamp& now);

    // 确保cancel在单一的eventLoop线程中执行
    void cancelInLoop(const TimerId& timerId);

    EventLoop* ownerloop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;
    TimerList toRemove;
};

// namespace detail
// {
//
// struct timespec howMuchTimeFromNow(Timestamp when);
// int createTimerfd();
// void resetTimerfd(int timerId, Timestamp now);
//
// }

}
