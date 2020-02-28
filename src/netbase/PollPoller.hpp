/* 
* 版权声明: PKUSZ 216 
* 文件名称 : PollPoller.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: epoll
* 历史记录: 无 
*/
// PollPoller的工作流程如下：
// 1、从创建epoll文件描述符
// 2、初始化epoll事件列表
// 3、把事件处理器添加到轮询器中
//     3.1、把事件处理器添加到事件处理器列表中
//     3.2、创建一个epoll事件，把epoll事件的事件设置为事件处理器需要监听的事件，把epoll事件的数据部分设置为指向事件处理器
//     3.2、把事件处理器需要监听的事件告诉操作系统
// 4、开始轮询
//     4.1、事件发生之后epoll_wait返回，events_中存放了已经发生的事件
//     4.2、把已经激活事件的事件处理器添加到已激活事件处理器列表中
//     4.3、进行事件处理
#pragma once

#include <netbase/Timestamp.hpp>
// #include <arbitration/netbase/EventLoop.hpp>
#include <logger/Logger.hpp>
#include <netbase/Poller.hpp>

#include <vector>
#include <map>

// 前向声明
struct pollfd;

namespace netbase
{

class Channel;
class EventLoop;

// 每一个Poller都有一个owner eventloop，所有操作都在一个线程里，故无需加锁
class PollPoller : public Poller
{
public:
    // typedef std::vector<Channel *> ChannelList;

    PollPoller(EventLoop* loop);
    ~PollPoller();
    // 轮询 
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    // 更新事件处理器 
    virtual void updateChannel(Channel* channel) override;
    // 移除事件处理器 
    virtual void removeChannel(Channel* channel) override;

    PollPoller(const Poller&) = delete;
    PollPoller& operator=(const PollPoller&) = delete;

private:
    typedef std::vector<struct pollfd> PollFdList;
    // 填充已激活的事件处理器列表
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    PollFdList pollfds_;
};

}
