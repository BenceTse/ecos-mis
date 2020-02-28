/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Channel.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 事件处理器
* 历史记录: 无 
*/

#include <assert.h>

#include "Channel.hpp"
#include "EventLoop.hpp"
#include <poll.h>
#include <functional>

#ifndef NODEBUG
#include <map>
#include <string>
#endif

namespace netbase
{

// 静态const成员变量可以在类声明里直接初始化，然而非const静态成员变量必须在类声明外初始化
// 初始化的方式是书写一遍类型的定义
/*
    poll 识别三种数据：
        - 普通：（TCP或UDP数据、TCP读半部关闭时（如收到对端的FIN、TCP连接存在错误、监听套接字有新连接）
        - 优先级带：TCP的带外数据
        - 高优先级：监听套接字上有新的连接可用

    POLLIN: 普通或优先级带数据可读
    POLLPRI: 高优先级数据可读
    POLLOUT: 普通数据可写
*/
const int Channel::kNoneEvent = 0;
// 读事件
const int Channel::kReadEvent = POLLIN | POLLPRI;
// 写事件
const int Channel::kWriteEvent = POLLOUT;
// 构建事件处理器 
Channel::Channel(EventLoop* loop, int fdArg)
    : loop_(loop),
      fd_(fdArg),
      events_(kNoneEvent),
      revents_(kNoneEvent),
      index_(-1),
      tied_(false),
      eventHandling_(false),
      addedInLoop_(false)
{
}
// 销毁事件处理器
Channel::~Channel()
{
    assert(!eventHandling_);
    assert(!addedInLoop_);
    CHECK(!eventHandling_) << "handling events, do not cancel me.";

    if(loop_->isInLoopThread()) {
        assert(!loop_->hasChannel(this));
    }
    // assert(!eventHandling_);

    // 暂时不要再析构里做，也许remove后又update到poller里
    // if(!isNoneEvent())
    //     disableAll();
    // remove();
}

void Channel::update()
{
    addedInLoop_ = true;
    loop_->updateChannel(this);
}
// 从Reactor中移除它自己 
void Channel::remove()
{
    addedInLoop_ = false;
    loop_->removeChannel(this);
    // addedInLoop_ = false;
}
// 把一个对象依附到本channel上
// 只要在执行handleEvent过程中，该对象就绝不会析构
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        // 持有guard的scope中，对象永远不会被析构
        // weak_ptr.lock() is thread safe
        std::shared_ptr<void> guard = tie_.lock();
        if(guard) {
            // 处理事件
            handleEventWithGuard(receiveTime);
        }
    } 
    else {
        handleEventWithGuard(receiveTime);
    }
}

#ifndef NODEBUG
static std::map<int, std::string> eventMap {
    {POLLIN, "POLLIN"},
    {POLLRDNORM, "POLLRDNORM"},
    {POLLRDBAND, "POLLRDBAND"},
    {POLLPRI, "POLLPRI"},
    {POLLOUT, "POLLOUT"},
    {POLLWRNORM, "POLLWRNORM"},
    {POLLWRBAND, "POLLWRBAND"},
    {POLLERR, "POLLERR"},
    {POLLHUP, "POLLHUP"},
    {POLLNVAL, "POLLNVAL"}
};
#endif

void Channel::handleEventWithGuard(Timestamp receiveTime)
{

#ifndef NODEBUG
//     std::string eventsStr;
//     for(auto it = eventMap.begin(); it != eventMap.end(); ++it)
//     {
//         if(revents_ & it->first)
//             eventsStr += it->second + " ";
//     }
//     LOG(DEBUG) << "Channel::handleEventWithGuard() There are events: " << eventsStr;
#endif

    eventHandling_ = true;
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        LOG(INFO) << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        if(closeCallBack_) closeCallBack_();
    }
    // POLLNVAL: 描述符不是一个打开的文件
    if(revents_ & POLLNVAL) {
        LOG(WARN) << "Channel::handleEvent() POLLNVAL";
    }
    // 出错 
    if(revents_ & (POLLERR | POLLNVAL)) {
        if(errorCallBack_) errorCallBack_();
    }
    // 可读
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallBack_) readCallBack_(receiveTime);
    }
    // 可写
    if(revents_ & POLLOUT) {
        if(writeCallBack_) writeCallBack_();
    }
    eventHandling_ = false;
}

std::string Channel::reventsToString() const
{
  return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const
{
  return eventsToString(fd_, events_);
}

std::string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str().c_str();
}

}
