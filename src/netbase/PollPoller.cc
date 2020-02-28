/* 
* 版权声明: PKUSZ 216 
* 文件名称 : PollPoller.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: epoll
* 历史记录: 无 
*/

#include <netbase/PollPoller.hpp>
#include <netbase/Channel.hpp>
#include <netbase/EventLoop.hpp>
#include <assert.h>

#include <poll.h>

namespace netbase
{

void printChannels(std::map<int, Channel*> channels)
{
    std::cout << "--------------------------------------------" << std::endl;
    for(auto it = channels.begin(); it != channels.end(); ++it)
    {
        Channel* ch = it->second;
        std::cout << "key(fd): " << it->first 
                  << "\tch->fd(): " << ch->fd() 
                  << "\tch->index(): " << ch->index()
                  << std::endl;
    }
    std::cout << "--------------------------------------------" << std::endl;
}

PollPoller::PollPoller(EventLoop* loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // printChannels(channels_);
    assert(pollfds_.size() > 0);
    // 调用epoll的等待函数，等待事件的发生  
    // epoll_wait函数返回之后，events_中存放这已经发生的事件
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
    Timestamp now(Timestamp::now());
    if(numEvents > 0) {
        // 填充已激活事件处理器的列表 
        fillActiveChannels(numEvents, activeChannels);
        // LOG(TRACE) << (*activeChannels)[0]->fd() << numEvents << " events happended";
        LOG(TRACE) << numEvents << " events happended";
    } else if(numEvents == 0) {
        LOG(TRACE) << "nothing happended";
    } else {
        LOG(ERROR) << "Poller::poll()";
    }
    return now;
}
// 填充已激活事件处理器的列表 
void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    // 把发生了事件的事件处理器加入到已激活的事件处理器列表中 
    for(PollFdList::const_iterator pfd = pollfds_.begin(); 
        pfd != pollfds_.end() && numEvents > 0;
        ++pfd)
    {
        if(pfd->revents > 0)
        {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second;
            // assert(channel->fd() == pfd->fd);
            CHECK(channel->fd() == pfd->fd) 
                << "channel->fd(): " << channel->fd() 
                << "\tpfd->fd: " << pfd->fd
                << "\t~pfd->fd: " << ~pfd->fd;

            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);

        }
    }
}
// 更新，其实是对往轮询器中删除、增加事件处理器  
// 操作的方式取决于Channel的index字段，默认是-1，表示新增  
void PollPoller::updateChannel(Channel* channel)
{
    assertInLoopThread();
    LOG(TRACE) << "PollPoller::updateChannel fd = " << channel->fd() 
        << " events = " << channel->events();
    if(channel->index() < 0)
    {
        assert(channels_.find(channel->fd()) == channels_.end());

        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);

        // ?
        int idx = pollfds_.size() - 1;
        channel->set_index(idx);

        channels_[pfd.fd] = channel;
    } else {
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);

        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if(channel->isNoneEvent()) {
            // ignore this pollfd
            // pollfd.events = 0 无法屏蔽POLLERR事件，所以要减少1
            // 不用的时候讲pfd.fd设置为其相反数减去1，刚好用取反符号即可
            pfd.fd = ~pfd.fd;
        }
    }
}
// 移除事件处理器  
void PollPoller::removeChannel(Channel* channel)
{
    ownerLoop_->assertInLoopThread();

    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    if(idx != static_cast<int>(pollfds_.size() - 1)) {
        // 如果channel对应的pollfd在list中不是最后一个，则跟最后一个交换
        std::swap(pollfds_[idx], pollfds_[pollfds_.size() - 1]);

        // pollfds_中每一个pollfd，其fd要不就是本身，要不就是本身的相反数
        int fd = pollfds_[idx].fd;
        fd = fd >= 0 ? fd : ~fd;
        // 找到fd后再channels_中修改index
        assert(fd >= 0 && channels_.find(fd) != channels_.end());
        channels_[fd]->set_index(idx);
    }
    pollfds_.pop_back();

    assert(channels_.find(channel->fd()) != channels_.end());
    // 对于map，erase操作返回值只可能是0或1
    int ret = channels_.erase(channel->fd());
    assert(ret == 1); (void)ret;
}

}
