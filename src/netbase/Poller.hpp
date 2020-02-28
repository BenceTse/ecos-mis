/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Poller.hpp
* 创建日期: 2018/03/04 
* 文件描述: 轮询器，通常一个Reactor对应一个轮询器，轮询器用于等待事件的发生
* 历史记录: 无 
*/
#pragma once

// #include <arbitration/netbase/Timestamp.hpp>
// #include <netbase/EventLoop.hpp>

#include <vector>
#include <map>

#include <netbase/Timestamp.hpp>

namespace netbase
{

class Channel;
class EventLoop;

class Poller
{
public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    ~Poller();
    // 轮询，通常是调用select、poll、epoll_wait等函数  
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    // Channel::update() --> EventLoop::updateChannel() --> Poller::updateChannel()
    // 负责维护和更新pollfds_
    // 更新事件处理器（通常是要处理的事件发生改变时调用） 
    virtual void updateChannel(Channel* channel) = 0;

    // 时间复杂度O(logN)
    // 移除事件处理器 
    virtual void removeChannel(Channel* channel) = 0;

    // 查看是poller是否有对应的channel
    virtual bool hasChannel(Channel* channel) const;

    void assertInLoopThread() const;
    // {
    //     ownerLoop_->assertInLoopThread();
    // }
    // 创建一个默认的轮询器
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    // 文件描述符和事件处理器的映射
    typedef std::map<int, Channel*> ChannelMap;
    // 存放事件处理器的容器
    ChannelMap channels_;
    // 所属的Reactor
    EventLoop* ownerLoop_;

// private:
    // EventLoop* ownerLoop_;

};

}
