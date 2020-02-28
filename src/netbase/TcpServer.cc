/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TcpServer.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: TcpServer
* 历史记录: 无 
*/
#include <netbase/TcpServer.hpp>
#include <netbase/EventLoop.hpp>
#include <netbase/SocketsOps.hpp>
#include <netbase/TcpConnection.hpp>
#include <netbase/EventLoopThread.hpp>
#include <netbase/EventLoopThreadPool.hpp>
#include <logger/Logger.hpp>
#include <assert.h>

namespace netbase
{

void defaultConnectionCallback(const TcpConnectionPtr& connPtr)
{
    LOG(TRACE) << "defaultConnectionCallback: a new connection. ";
}

void defaultMessageCallback(const TcpConnectionPtr& connPtr, Buffer* buf, Timestamp receiveTime)
{
    LOG(TRACE) << "defaultMessageCallback: message came";
    // ::read(connPtr->buf, n);
}

void defaultWriteCompleteCallback(const TcpConnectionPtr& connPtr)
{
    LOG(TRACE) << "defaultWriteCompleteCallback: write complete";
}

void defaultHighWaterMarkCallback(const TcpConnectionPtr& connPtr, size_t len)
{
    LOG(TRACE) << "defaultHighWaterMarkCallback: too much data, actually " << len 
               << " bytes remained, and unset HighWaterMarkCallback";
}

void mThreadInitCallback(EventLoop* loop)
{
    // LOG(TRACE) << "
}
/* 
 * 构造函数 
 */  
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
    : loop_(loop),
      name_(name),
      acceptor_(new Acceptor(loop, listenAddr)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      writeCompleteCallback_(defaultWriteCompleteCallback),
      highWaterMarkCallback_(defaultHighWaterMarkCallback),
      started_(),       // 默认为0
      nextConnId_(1),   // ??
      // nloops_(0),
      ioLoops_(new EventLoopThreadPool(loop, name))
{
    assert(loop_ != nullptr);
    // 创建Acceptor
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, 
                                                  this, 
                                                  std::placeholders::_1, 
                                                  std::placeholders::_2));
    // acceptor_.setNewConnectionCallback
}

TcpServer::~TcpServer()
{
    for (ConnectionMap::iterator it(connections_.begin());
         it != connections_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();
      }
}
/* 
 * 设置EventLoop线程池中线程的数量 
 */ 
void TcpServer::setThreadNums(size_t n)
{
    ioLoops_->setThreadNum(n);
}
// 启动服务器
void TcpServer::start()
{
    if(started_.getAndSet(1) == 0)
    {
        // ioLoops_ = new EventLoopThreadPool(nloops_, name_, mThreadInitCallback);
        // 启动线程池中的线程
        ioLoops_->start();

        assert(!acceptor_->listenning());
        // Acceptor开始监听 
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));

    }
}
/* 
 * 新链接到来的回调函数 
 */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_++);
    std::string connName = name_ + buf;
    InetAddress localAddr(wrap_sockets::getLocalAddr(sockfd));

    LOG(INFO) << "TcpConnection::newConnection [" << name_
              << "] - new connection [" << connName
              << "] from " << peerAddr.toIpPort();
    
    EventLoop* ioloop = ioLoops_->getNextLoop();
    if(ioloop == nullptr)
        ioloop = loop_;

    TcpConnectionPtr conn(new TcpConnection(ioloop, connName, 
                sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    // conn的回调在连接建立前确定，从此以后不会再改动
    // 改变tcpserver的回调只能改变从此以后的新建的tcpconnection
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setHighWaterMarkCallback(highWaterMarkCallback_);
    // 不能这样写，否则cb会持有connection的shared_ptr，而cb又是connnection本身的成员变量
    // 这导致connection永远不能被析构
    // conn->setCloseCallback(std::bind(&TcpServer::removeConn, this, conn); ??
    conn->setCloseCallback(std::bind(&TcpServer::removeConn, this, 
                std::placeholders::_1));

    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
    // conn->connectEstablished();

}
// 移除一个连接 
void TcpServer::removeConn(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnInLoop, this, conn));
}

void TcpServer::removeConnInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG(INFO) << "TcpServer::removeConn connection [" << conn->name()
              << "] is removing";
    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n==1);

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}
