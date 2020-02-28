/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Timer.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 定时器
* 历史记录: 无 
*/
#pragma once
#include <netbase/Atomic.h>
#include <netbase/Callbacks.hpp>
#include <netbase/Timestamp.hpp>

namespace netbase
{

class Timer
{
public:
    // callback: 回调函数，到时会自动执行
    // Timerstamp: 指定时间戳，到时会执行回调函数
    // interval: 时间间隔，若为0则，不会重复；若大于0，则会重复执行
    Timer(const TimerCallback&& callback, Timestamp when, double interval)
        // : callback_(std::move(callback)),
        : callback_(callback),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCreated_.incrementAndGet())
    {
    }

    void run() const;
    // void run() const
    // {
    //     // assert(callback_);
    //     CKECK()
    //     callback_();
    // }

    // 获取逾期时间点
    Timestamp expiration() const { return expiration_; }
    // 是否重复
    bool repeat() const { return repeat_; }
    // 唯一的序列号
    int64_t sequence() const { return sequence_; }

    // 重启该定时器
    void restart(Timestamp now);

    // 获取唯一的序列号
    static int64_t numCreated() { return s_numCreated_.get(); }
    
    // 不可拷贝
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

private:
    //定时器回调函数
    const TimerCallback callback_;
    //到期时间
    Timestamp expiration_;
    //周期长度
    const double interval_;
    //是否为周期计时器
    const bool repeat_;
    //计时器序号
    const int64_t sequence_;
    //创建的计时器个数
    static AtomicInt64 s_numCreated_;
};
/* 
 * 定时器（计时器）ID类 
 * 主要的作用是用于撤销定时器，被TimerQueue所使用 
 * TimerQueue使用TimerId的sequence_或timer_来标识一个定时器 
 */
class TimerId
{
public:
    TimerId()
        : timer_(nullptr),
          sequence_(0)
    {
    }

    TimerId(Timer* timer, int64_t seq)
        : timer_(timer),
          sequence_(seq)
    {
    }

    bool isInvalid() { return timer_ == nullptr; }

    friend class TimerQueue;
private:
    Timer* timer_;
    int64_t sequence_;
};

}
