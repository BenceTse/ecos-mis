/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TcpConnection.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: TCP客户端与TCP服务器之间的一个连接
* 历史记录: 无 
*/
#include <netbase/TcpConnection.hpp>
#include <netbase/Buffer.hpp>
#include <netbase/EventLoop.hpp>
#include <unistd.h> // delete it
#include <assert.h>

#include <netbase/SocketsOps.hpp>

namespace netbase
{
/* 
 * 构造函数 
 */ 
TcpConnection::TcpConnection(EventLoop* loop, const std::string& name,
                             int sockfd, const InetAddress& localAddr, 
                             const InetAddress& peerAddr)
    : loop_(loop),
      name_(name),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
      // highWaterMark_(1024)
{
    // 设置事件处理器的回调函数
    channel_->setReadCallBack(std::bind(&TcpConnection::handleRead, 
                this, std::placeholders::_1)); 
    // FIXME: add the other callback
    channel_->setWriteCallBack(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorCallBack(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));

    LOG(DEBUG) << "TcpConnection::c'tor[" << name_ << "] at " 
        << this << " fd=" << sockfd;
    // what does it mean?
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG(TRACE) << "TcpConnection::dtor[" <<  name_ << "] at " << this
               << " fd=" << channel_->fd()
               << " state=" << state_;
    assert(state_ == kDisconnected);
}

void TcpConnection::forceClose() {
    if(state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        // runInLoop如果正好在loop线程中，会直接执行下述语句
        // 有些时候也许handleEvent时还有其它对本conn的操作
        // 延后它们执行是防止close后却发现还有
        // 一些channel跟这个tcp相关的event还未执行
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, 
                    shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if(state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0) {
        if(messageCallback_)
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        // 对端关闭连接时，这端会触发读操作，并只能读入0个字节
        handleClose();
    } else {
        errno = savedErrno;
        LOG(ERROR) << "TcpConnection::handleRead()";
        handleError();
    }
}
 /* 处理关闭，它和shutdownInLoop的区别在于:
  *     shutdownInLoop是主动（自己）关闭连接 
  *     handleClose是对方关闭了连接 
  */ 
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();  
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);

    channel_->disableAll();

    // 如果handleClose只能在loop执行，那么这段代码
    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);

    if(closeCallback_) {
        closeCallback_(guardThis);
    }
    // new
    // channel_->remove();
}

void TcpConnection::handleError()
{
  int err = wrap_sockets::getSocketError(channel_->fd());
  LOG(WARN) << "TcpConnection::handleError [" << name_
             << "] - SO_ERROR = " << err << " " << strerror(err);
}
// 连接完成，当tcp服务器接收到一个新的连接时调用它
void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    if(connectionCallback_)
        connectionCallback_(shared_from_this());
}
// 连接已经被销毁，当一个连接被服务器移除的时候
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread(); 
    // assert(state_ == kConnected);
    if(state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        if(connectionCallback_)
            connectionCallback_(shared_from_this());
    }
    channel_->remove();
}
// 主动关闭 
void TcpConnection::shutdown()
{
    // assert(connected());
    if(connected()) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}
// 主动关闭 
void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if(outputBuffer_.writableBytes() == 0)
    {
        socket_->shutdownWrite();
    }
}
// 发送数据
void TcpConnection::send(Buffer* buf) {
    assert(connected());
    if(connected())
    {
        if(loop_->isInLoopThread())
            sendInLoop(*buf);
        else {
            // 
            auto binder = std::bind(static_cast<void(TcpConnection::*)(Buffer&)>(&TcpConnection::sendInLoop),
                                    this,
                                    std::move(*buf));
            // 不知为何此处一定要Buffer有拷贝构造函数 ?
            loop_->queueInLoop(std::move(binder));
            // loop_->queueInLoop(std::bind(
            //             static_cast<void(TcpConnection::*)(Buffer&)>(&TcpConnection::sendInLoop),
            //             this,
            //             std::move(*buf)));
                        // *buf));
        }
    }
}
// 发送数据
void TcpConnection::send(const std::string& message)
// void TcpConnection::send(std::string&& message)
{
    // assert(connected());
    if(connected())
    {
        if(loop_->isInLoopThread())
            sendInLoop(message);
        else
            loop_->queueInLoop(std::bind(static_cast<void(TcpConnection::*)(
                            const std::string&)>(&TcpConnection::sendInLoop), 
                            this, 
                            std::move(message)));
    }
}

void TcpConnection::sendInLoop(Buffer& buf) {
    sendInLoop(buf.peek(), buf.readableBytes());
    buf.retrieveAll();
}

void TcpConnection::sendInLoop(const std::string& message) {
    sendInLoop(message.c_str(), message.size());
}

void TcpConnection::sendInLoop(const void* ptr, size_t len)
{
    loop_->assertInLoopThread();
    // assert(connected());
    if(state_ == kDisconnected) {
        LOG(WARN) << "disconnected, give up writing";
        return ;
    }

    bool faultError = false;
    // const char* msgPtr = message.c_str();
    // 尽可能早地发出数据
    ssize_t nwrote = 0;
    if(outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), ptr, len);
        if(nwrote < 0) {
            if(errno != EWOULDBLOCK) {
                LOG(ERROR) << "TcpConnection::sendInLoop ::write error";
                if(errno == EPIPE || errno == ECONNRESET)
                    faultError = true;
            }
            nwrote = 0;
        } else if(static_cast<size_t>(nwrote) == len && writeCompleteCallback_) {
            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
    }
    // 如果还有残留的数据没有发送完成
    assert(nwrote >= 0);
    if(!faultError && static_cast<size_t>(nwrote) < len) {
        // 高水位回调只触发一次
        size_t remain = len - nwrote;
        size_t oldlen = outputBuffer_.readableBytes();
        // 如果输出缓冲区的数据已经超过高水位标记，那么调用highWaterMarkCallback_
        if(remain + oldlen >= highWaterMark_ 
           && oldlen < highWaterMark_
           && highWaterMarkCallback_) 
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), remain + oldlen));
        }

        outputBuffer_.append(static_cast<const char*>(ptr) + nwrote, len - nwrote);
        // 把数据添加到输出缓冲区中 
        if(!channel_->isWriting())
            channel_->enableWriting();
    }
}

/* 
 * 处理写事件 
 */
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread(); 
    if(channel_->isWriting()) {
         // 写数据
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        LOG(TRACE) << "TcpConnection::handleWrite writen " << n << " bytes ";
        // 正确写入
        if(n < 0) {
            if(errno != EWOULDBLOCK) {
                LOG(ERROR) << "TcpConnection::handleWrite ::write error";
            }
        } else if(static_cast<size_t>(n) == outputBuffer_.readableBytes()) {
            // 如果不关掉，会造成busy loop
            channel_->disableWriting();
            // 移动输出缓冲区的指针 
            outputBuffer_.retrieveAll();
            if(state_ == kDisconnecting)
                socket_->shutdownWrite();

            if(writeCompleteCallback_) 
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));

        } else {
            outputBuffer_.retrieve(n);
            LOG(TRACE) << "TcpConnection::handleWrite going to write more data";
        }
    }
}

}
