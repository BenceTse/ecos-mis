/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TcpServer.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: TcpServer
* 历史记录: 无 
*/
/*
   Tcp服务器的封装
   关注四个事件：链接建立(connector)、链接断开、有数据读、数据发送完毕
       连接建立：Acceptor::listen(), Acceptor::handleRead()
            -->Acceptor::newConnectionCallback_() [ 也就是TcpConnection::connectionCallback_() ]
       连接断开：TcpConnection::handleRead()-->TcpConnection::handleClose()-->TcpConnection::closeCallback_()
            -->TcpServer::removeConn()-->TcpConnection::connectDestoryed()-->TcpConnection::connectionCallback_()
       有数据读：TcpConnection::handleRead()-->TcpConnection::messageCallback_()
       数据写完：TcpConnection::handleWrite()-->TcpConnection::writeCompleteCallback_()
 */
/*
TcpServer的工作流程如下：
1、根据构造函数传递进来的Acceptor的EventLoop和地址等信息进行初始化
2、创建Acceptor对象
3、把Accptor的EventLoop作为参数，创建EventLoop线程池，此时Acceptor的EventLoop就是线程池的基础EventLoop，就算线程池设置的线程数量为0，线程池中也还有一个基础的EventLoop。其中线程个数的意义如下：
4、为Acceptor设置新连接到来的回调函数
5、开始监听
6、新连接到来的时候，处理流程如下：
    6.1、从线程池中取出一个EventLoop对象，如果线程池的的线程数量（每创建一个线程就意味着多创建一个EventLoop）为0，那么就返回基础的EventLoop（即Accptor的EventLoop）。新的连接就交给这个EventLoop来管理。
    6.2、计算新连接的id和名字
    6.3、利用步骤6.1返回的EventLoop对象创建一个TcpConnection对象
    6.4、存储TcpConnection对象
    6.5、为TcpConnection对象设置用户的回调函数:连接到来的回调函数、数据到来的回调函数、写完成的回调函数、关闭连接的回调函数
    6.6、调用TcpConnection::connectEstablished表示链接建立完毕
*/
#pragma once

#include <netbase/Acceptor.hpp>
#include <netbase/Callbacks.hpp>
#include <netbase/Atomic.h>

#include <string>
#include <memory>
#include <map>
#include <vector>

namespace netbase
{

class EventLoop;
class EventLoopThreadPool;

class TcpServer
{
public:
    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name);
    ~TcpServer();

    // thread safe
    void start();

    // PS: 无论是新连接还是连接关闭，都会调用该回调。所以用户负责在该回调中判断是新连接还是连接关闭
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    // 网络库只负责接受数据，至于对方是否还有数据发过来，数据是不是被拆分了几个包，网络库并不管这个
    // 网络库只能保证有数据到来时，尽可能读取完数据，然后调用message回调
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb) { highWaterMarkCallback_ = cb; }

    void setThreadNums(size_t n);

    // 不可拷贝
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

private:
    // 新连接到来回调函数 
    void newConnection(int sockfd, const InetAddress& peerAddr);
    // 删除连接 
    void removeConn(const TcpConnectionPtr& conn);
    void removeConnInLoop(const TcpConnectionPtr& conn);

    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;
    // 接收者的Reactor对象 
    EventLoop* loop_;
    const std::string name_;
    // 接收者  
    std::unique_ptr<Acceptor> acceptor_;
    // 连接建立回调函数 
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    // 写完成回调函数
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    // 服务器是否已经启动
    AtomicInt32 started_;
    // 下一个连接的id
    int nextConnId_;
    // 存放所有的连接
    ConnectionMap connections_;
    
    // size_t nloops_;
    std::unique_ptr<EventLoopThreadPool> ioLoops_;
    // std::vector<std::unique_ptr<EventLoopThreadPool>> sdjfo;
};

}
