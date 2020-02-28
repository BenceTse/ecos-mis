/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Timer.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 定时器
* 历史记录: 无 
*/
#include <assert.h>
#include <netbase/Timer.hpp>

namespace netbase
{

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    //如果是周期性定时器，重新设置到期时间
    if(repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}

void Timer::run() const
{
    assert(callback_);
    callback_();
}

}
