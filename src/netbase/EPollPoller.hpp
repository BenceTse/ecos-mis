#pragma once

#include "Poller.hpp"

namespace netbase {

class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller();
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    // 更新事件处理器 
    virtual void updateChannel(Channel* channel) override;
    // 移除事件处理器 
    virtual void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize = 16;
    static const char* operationToString(int op);

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

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
    void update(int op, Channel* channel);

    typedef std::vector<struct epoll_event> EventList;

    int epollfd_;
    EventList events_;
};

}
