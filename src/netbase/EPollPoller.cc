#include <sys/epoll.h>
#include <assert.h>
#include <string.h>

#include <logger/Logger.hpp>
#include "EPollPoller.hpp"
#include "Channel.hpp"

namespace netbase {
// 使用channel->index 来保存状态
// channel 的默认构造函数中index 初值为 -1
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
    // int epoll_create(int size);
    // flag=0的时候，相当于size弃用了的epoll_create
    // int epoll_create1(int flag);
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if(epollfd_ < 0)
        LOG(FATAL) << "EPollPoller::EPollPoller";
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG(TRACE) << "fd total count" << channels_.size();
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), 
                                 // 最多能由内核放多少个events
                                 static_cast<int>(events_.size()), 
                                 timeoutMs);
    Timestamp now(Timestamp::now());
    if(numEvents > 0) {
        LOG(TRACE) << "At least " << numEvents << " events happened.";
        fillActiveChannels(numEvents, activeChannels);
        if(static_cast<size_t>(numEvents) == events_.size()) {
            // 如果发现：本次poll一次性填满了events_，说明开辟的空间可能不够
            // 该设计可以动态扩容，刚开始只能存16个fd的events
            events_.resize(events_.size() << 1);
        }
    } else if(numEvents == 0) {
        LOG(TRACE) << "nothing happened.";
    } else {
        LOG(ERROR) << "EPollPoller::poll()";
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NODEBUG
        // 开发版本，才会有这些检测开销
        ChannelMap::const_iterator it = channels_.find(channel->fd());
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::updateChannel(Channel* channel) {
    assertInLoopThread();
    const int index = channel->index();
    const int fd = channel->fd();
    (void)fd;
    if(index == kNew || index == kDeleted) {
        if(index == kNew) {
            // 说明channels_不存在该channel，需要执行EPOLL_CTL_ADD操作
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            // 已经有了该channel？
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        // 调用epoll_ctl添加
        update(EPOLL_CTL_ADD, channel);
    } else {
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if(channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel) {
    assertInLoopThread();
    int fd = channel->fd();
    // 如非必要，尽可能缩小范围，尽可能让程序往自己想的方向走，而不是采用
    // “补充”的方式来处理unexpected的状况
    assert(channel->isNoneEvent());
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    int index = channel->index();
    // kDeleted 需要吗？
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);
    if(index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// typedef union epoll_data {
//     void        *ptr;
//     int          fd;
//     uint32_t     u32;
//     uint64_t     u64;
// } epoll_data_t;
//
// struct epoll_event {
//     uint32_t     events;      [> Epoll events <]
//     epoll_data_t data;        [> User data variable <]
// };
void EPollPoller::update(int op, Channel* channel) {
    // 构造一个新的event，让其ptr指向channel，这样event与channel就能一一对应起来
    // 说明epoll_data_t只是给用户使用的，保存上下文信息的额外数据结构
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG(TRACE) << "epoll_ctl op = " << operationToString(op)
        << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    // 更改epoll的list表只是小频率操作
    if(::epoll_ctl(epollfd_, op, fd, &event) < 0) {
        if(op == EPOLL_CTL_DEL) {
            LOG(ERROR) << "epoll_ctl op =" << operationToString(op) << " fd =" << fd;
        } else {
            LOG(FATAL) << "epoll_ctl op =" << operationToString(op) << " fd =" << fd;
        }
    }
}

const char* EPollPoller::operationToString(int op) {
    switch(op) {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default: 
            assert(false && "error op");
            return "unknown operation";
    }
}

}
