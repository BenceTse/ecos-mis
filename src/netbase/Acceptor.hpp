/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Accerptor.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 接收者Accpetor
* 历史记录: 无 
*/
// Accpetor的作用如下：
// 1、创建监听（接收者）套接字
// 2、设置套接字选项
// 3、创建监听套接字的事件处理器，主要用于处理监听套接字的读事件（出现读事件表示有新的连接到来）
// 4、绑定地址
// 5、开始监听
// 6、等待事件到来

#pragma once
#include <functional>
#include <netbase/Channel.hpp>
#include <netbase/Socket.hpp>

namespace netbase
{

class EventLoop;
class InetAddress;

class Acceptor
{
public:
    // 新链接到来的回调函数  
    typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listenaddr);
    ~Acceptor();
    
    // 设置新链接到来的回调函数 
    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    // 是否正在监听
    bool listenning() const { return listenning_; }
    // 监听
    void listen();

    void removeChannel(Channel* channel);

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

private:
    // 处理读事件，acceptChannel_的读事件回调函数
    void handleRead();
    // 所属的Reactor
    EventLoop* loop_;
    // 监听/接收者套接字
    Socket acceptSocket_;
    // 监听套接字的事件处理器（主要处理读事件，即新链接到来的事件）;
    Channel acceptChannel_;
    // 新链接到来的回调函数
    NewConnectionCallback newConnectionCallback_;
    // 是否正在监听
    bool listenning_;
};

}
