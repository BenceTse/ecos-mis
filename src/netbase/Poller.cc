/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Poller.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 轮询器，通常一个Reactor对应一个轮询器，轮询器用于等待事件的发生
* 历史记录: 无 
*/
#include <netbase/Poller.hpp>
#include <netbase/PollPoller.hpp>
#include <netbase/EPollPoller.hpp>
#include <netbase/EventLoop.hpp>

namespace netbase
{
//构造函数
Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{
}
//析构函数
Poller::~Poller()
{
}

void Poller::assertInLoopThread() const
{
    ownerLoop_->assertInLoopThread();
}
// 创建一个默认的轮询器
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
#ifdef _USE_EPOLL_
    return new EPollPoller(loop);
#else
    return new PollPoller(loop);
#endif
}


bool Poller::hasChannel(Channel* channel) const {
    auto it = channels_.find(channel->fd());
    // fd对应的channel的只是当前的存在的
    // fd是可能被复用的，对应不同时间点下的channel
    return it != channels_.end() &&
           it->second == channel;
}

}
