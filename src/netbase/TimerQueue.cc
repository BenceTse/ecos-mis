/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TimerQueue.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 定时器队列 
* 历史记录: 无 
*/
#include <netbase/TimerQueue.hpp>
#include <netbase/EventLoop.hpp>
#include <netbase/Timer.hpp>
#include <logger/Logger.hpp>

#include <assert.h>

#include <sys/timerfd.h>
// for ::read
#include <unistd.h>

namespace netbase
{

namespace detail
{

// 从时间戳转换为struct timespec类型（符合人的自然习惯）
struct timespec howMuchTimeFromNow(Timestamp when)
{
    Timestamp now = Timestamp::now(); 
    int64_t microSeconds = when.microSecondsSinceEpoch() - now.microSecondsSinceEpoch();
    // 磨掉粗糙的时间点
    if(microSeconds < 100) 
        microSeconds = 100;

    struct timespec howMuch;
    howMuch.tv_sec = static_cast<time_t>(microSeconds / Timestamp::kMicroSecondsPerSecond);
    // howMuch.tv_nsec = static_cast<long>((microSeconds * 1000) % (Timestamp::kMicroSecondsPerSecond * 1000));
    howMuch.tv_nsec = static_cast<long>((microSeconds % Timestamp::kMicroSecondsPerSecond) * 1000);

    return howMuch;
}

// 封装::timerfd_create，确保处理得到
int createTimerfd()
{
    int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(fd < 0)
        LOG(FATAL) << "timerfd_create() error!";
    return fd;
}

// expiration == timestamp::invalid() 意味着关掉timerfd
void resetTimerfd(int timerfd, Timestamp expiration)
{
    // LOG(DEBUG) << "TFD_TIMER_ABSTIME: " << TFD_TIMER_ABSTIME;
    struct itimerspec newits, oldits;
    bzero(&newits, sizeof(newits));

    if(expiration != Timestamp::invalid()) {
        newits.it_value = howMuchTimeFromNow(expiration);
    }

    // 此处expiration采用相对值
    int ret = ::timerfd_settime(timerfd, 0, &newits, &oldits);
    if(ret)
        LOG(ERROR) << "timerfd_settime error";
}

// 电平触发，必须清空读缓冲区
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    LOG(TRACE) << "TimerQueue::handleRead() " << howmany << " times " << " at " << now.toString();
    if(n != sizeof(howmany)) {
        LOG(ERROR) << "read " << n << " bytes instead of 8 bytes";
    }
}

void printTimersInfo(std::set<std::pair<Timestamp, Timer*> > timerList)
{
    for(auto it = timerList.cbegin(); it != timerList.cend(); ++it)
    {
        LOG(DEBUG) << it->first.microSecondsSinceEpoch() << "\t" << it->second;
    }
}

}

using namespace netbase;
using namespace netbase::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : ownerloop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_()
{
    timerfdChannel_.setReadCallBack(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    // while(1);
    // 不再接受
    timerfdChannel_.disableAll();
    // not implemented
    timerfdChannel_.remove();

    ::close(timerfd_);

    // 所有未到期的定时器都要删除
    for(auto it = timers_.begin(); it != timers_.end(); ++it)
    {
        delete (*it).second;
    }
    // while(1);

}

// void TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval)
TimerId TimerQueue::addTimer(const TimerCallback&& cb, Timestamp when, double interval)
{
    // Timer的第一个参数是右值引用类型，只能绑定到左值上，而cb是一个右值引用类型的变量（实际是左值）
    Timer* timer = new Timer(std::move(cb), when, interval);
    ownerloop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    // ownerloop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;

    {
        std::pair<TimerQueue::TimerList::iterator, bool> result = 
            timers_.insert(std::make_pair(timer->expiration(), timer));
        assert(result.second);
        // 抑制noused warning
        (void) result;
    }

    if(timer == timers_.begin()->second)
        earliestChanged = true;

    return earliestChanged;
}

void TimerQueue::handleRead()
{
    ownerloop_->assertInLoopThread();
    LOG(TRACE) << "Timer expired";
    // 读，刷新读缓存区
    Timestamp now = Timestamp::now(); 
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);
    // reset(expired, now);
    for(auto it = expired.begin(); it != expired.end(); ++it)
    {
        it->second->run();
    }
    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    ownerloop_->assertInLoopThread();
    std::vector<TimerQueue::Entry> expired;
    // 设置哨兵值
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    // 拷贝所有逾期事件
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
}

void TimerQueue::reset(std::vector<Entry>& expired, Timestamp& now)
{
    ownerloop_->assertInLoopThread();
    for(auto it = expired.begin(); it != expired.end(); ++it)
    {
        // 所有toRemove的timer都需要删除，无论是否是重复的
        auto pos = toRemove.find(*it);
        Timer* timer = it->second;
        // 平衡二叉树维护下，查找和删除都在对数时间内完成
        if(pos != toRemove.end()) {
            toRemove.erase(pos);
            delete timer;
        } else {
            // 如果是重复事件，则需要重新进入timerlist
            if(timer->repeat()) {
                auto pos = toRemove.find(*it);
                if(pos != toRemove.end()) {
                    toRemove.erase(pos);
                }
                
                timer->restart(now);
                std::pair<std::set<Entry>::iterator, bool> result = 
                    timers_.insert(std::make_pair(timer->expiration(), timer));
                assert(result.second);
                (void) result;
            } else {
                delete timer;
            }
        }
    }
    // assert(toRemove.empty());

    // 如果timerlist不为空，则取最近的timer的逾期点重新设定定时器
    if(!timers_.empty()) { 
        Timestamp nextExpire = timers_.begin()->first;
        assert(nextExpire.valid());
        resetTimerfd(timerfd_, nextExpire);
    }

    toRemove.clear();
}

void TimerQueue::cancel(const TimerId& timerId)
{
    ownerloop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(const TimerId& timerId)
{
    // 确保与eventloop同在一个线程
    ownerloop_->assertInLoopThread(); 
    CHECK(timerId.timer_ != nullptr) << "Default TimerId can be canceled";
    auto element = std::make_pair(timerId.timer_->expiration(), timerId.timer_);
    auto pos = timers_.find(element);

    // assert or if ??
    // assert(pos != timers_.end());
    // assert(pos->second->sequence() == timerId.sequence_);

    if(pos != timers_.end())
    {
        // 如果删去的定时器恰好是timerlist中的第一个，则需要重新设置定时器
        assert(pos->second->sequence() == timerId.sequence_);
        if(pos == timers_.begin())
        {
            if(timers_.size() == 1) {
                resetTimerfd(timerfd_, Timestamp::invalid());
            } else {
                auto nextPos = pos;
                nextPos++;
                Timestamp nextExpire = nextPos->first;
                resetTimerfd(timerfd_, nextExpire);
            }
        }
        timers_.erase(pos);
    } else {
        std::pair<TimerList::iterator, bool> result = toRemove.insert(element);
        assert(result.second);
        (void) result;
    }
}

}
