/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Channel.hpp
* 创建日期: 2018/03/04 
* 文件描述: 事件处理器
* 历史记录: 无 
*/
/*  
 * 对一个套接字（也可以是其他类型的文件描述符）进行包装，处理该套接字发生的事件 
 * EventLoop收到一个事件之后，就会调用 套接字对应的Channel 来进行处理 
 */
#pragma once

#include <functional>
#include <memory>

namespace netbase{

// 前向声明
class EventLoop;
class Timestamp;

/*!
 *  @class Channel
 *  @brief IO多路复用器拿到IO事件后分发给各个描述符，关键串联类
 *
 *  Detailed description
 *         Channel 不持有文件描述符，不负责描述符关闭等操作
 *         Channel 在析构的时候也不会remove掉自己，实际上什么事都不做
 *         全部由持有者负责清理
 *         文件描述符可以是：eventfd, timerfd, signalfd
 */
class Channel
{
public:
    // 事件回调函数
    typedef std::function<void()> EventCallback;
    // 读事件的回调函数
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int fdArg);
    ~Channel();
    // 处理事件
    void handleEvent(Timestamp receiveTime);
    // 设置读回调函数  
    void setReadCallBack(const ReadEventCallback& cb) { readCallBack_ = cb; }
    // 设置写回调函数
    void setWriteCallBack(const EventCallback& cb) { writeCallBack_ = cb; }
    // 设置错误处理回调函数 
    void setErrorCallBack(const EventCallback& cb) { errorCallBack_ = cb; }
    // 设置关闭连接时的回调函数
    void setCloseCallback(const EventCallback& cb) { closeCallBack_ = cb; }
    // 返回文件描述符
    int fd() const { return fd_; }
    // 返回该事件处理所需要处理的事件
    int events() const { return events_; }
    // 设置事件
    void set_revents(int revt) { revents_ = revt; }
    // 判断是否有事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    // 启用读
    void enableReading() { events_ |= kReadEvent; update(); }
    // 启用写
    void enableWriting() { events_ |= kWriteEvent; update(); }
    // 禁用写
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    // 是否正在写
    bool isWriting() { return events_ & kWriteEvent; }

    void remove();

    // for std::shared_ptr 
    // 把当前事件处理器依附到某一个对象上
    void tie(const std::shared_ptr<void>& obj);

    // for Poller
    // 返回索引
    int index() { return index_; }
    // 设置索引
    void set_index(int idx) { index_ = idx; }
    // 所属的事件循环 
    EventLoop* ownerLoop() { return loop_; }

      // for debug
    std::string reventsToString() const;
    std::string eventsToString() const;

    // non-copyable
    Channel(const Channel&) = delete;
    Channel operator=(const Channel&) = delete;

private:
    static std::string eventsToString(int fd, int ev);
    // 更新
    void update();
    // 处理事件
    void handleEventWithGuard(Timestamp receiveTime);
    // 没有事件
    static const int kNoneEvent;
    // 读事件
    static const int kReadEvent;
    // 写事件
    static const int kWriteEvent;

    // channel的成员变量无需加锁保护，因为channel只属于一个EventLoop线程
    // 即只可能在在同一个IO线程内访问
    EventLoop* loop_;
    // 该事件处理器对应的文件描述符
    const int fd_;
    // 当前的事件 
    int events_;
    // 事件类型
    int revents_;
    // 索引
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;
    // 是否正在处理事件
    bool eventHandling_;
    // 是否已经加入到loop的poller中
    bool addedInLoop_;
    // 读事件的回调
    ReadEventCallback readCallBack_;
    // 关闭事件的回调
    EventCallback writeCallBack_;
    EventCallback errorCallBack_;
    EventCallback closeCallBack_;
};

}
