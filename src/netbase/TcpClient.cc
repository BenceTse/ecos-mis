/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TClient.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: Tcp客户端封装，构造时必须指定所在的EventLoop
* 历史记录: 无 
*/
#include <netbase/TcpClient.hpp>
#include <netbase/EventLoop.hpp>
#include <netbase/SocketsOps.hpp>
#include <assert.h>

namespace netbase
{

namespace detail {

void removeConnection(EventLoop *loop, const TcpConnectionPtr& conn) {
    // runInLoop如果正好在loop线程中，会直接执行下述语句
    // 有些时候也许handleEvent时还有其它对本conn的操作
    // 延后它们执行是有意义的
    // conn->connectDestroyed();
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

}
/* 
 * 构造函数 
 */ 
TcpClient::TcpClient(EventLoop* loop,
          const InetAddress& servAddr,
          const std::string& nameArg)
    : loop_(loop),
      servAddr_(servAddr),
      name_(nameArg),
      connector_(new Connector(loop, servAddr)),// 创建一个连接器
      isConnected(false)
{
    // 设置连接器的连接完成回调函数
    connector_->setNewConnectionCallback(std::bind(
                &TcpClient::newConnectionCallback, 
                this, 
                std::placeholders::_1));
}

/* 
 * 析构函数 
 */
TcpClient::~TcpClient()
{
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if(conn) {
        // 回收的connection与本对象所在的loop必然要一致？
        assert(loop_ == conn->getLoop());
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, 
                std::placeholders::_1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if(unique) {
            conn->forceClose();
        }
    } else {
        // 说明还没有建立好连接，连接器应该立刻停止
        connector_->stop();
        loop_->runAfter(1.0, std::bind(&detail::removeConnector, connector_));
    }
}

// 连接到服务器
void TcpClient::connect()
{
    // 开始连接 
    connector_->start();
}
// 断开连接
void TcpClient::disconnect()
{
    isConnected = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(connection_)
        {
            connection_->shutdown();
        }
    }

    /*
   if(isConnected)
   {
       isConnected = false;
       loop_->runInLoop(std::bind(&TcpConnection::connectDestroyed, 
                        connection_));
   }
   */
}

void TcpClient::reconnect() {
    disconnect();
    connector_->restart();
}

void TcpClient::newConnectionCallback(int sockfd)
{
    InetAddress localAddr(wrap_sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, 
            name_, sockfd, localAddr, servAddr_);
    
    if(connectionCallback_)
        conn->setConnectionCallback(connectionCallback_);
    // 决不能这样，会导致tcpConnectionPtr永远无法析构
    // connection_->setConnectionCallback(std::bind(connectionCallback_, tcpConnection));
    if(messageCallback_)
        conn->setMessageCallback(messageCallback_);
    if(writeCompleteCallback_)
        conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, 
                this, std::placeholders::_1));
    {
        std::lock_guard<std::mutex> lk(mutex_);
        bool unique = connection_.unique();
        if(connection_) {
            CloseCallback cb = std::bind(&detail::removeConnection, 
                    loop_, std::placeholders::_1);
            loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, 
                                       connection_, cb));
            if(unique) connection_->forceClose();
            // connection_->forceClose();
        }
        connection_ = conn;
    }
    
    conn->connectEstablished();

    isConnected = true;
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());
    CHECK(loop_ == conn->getLoop()) << "something wrong must happen";

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}
