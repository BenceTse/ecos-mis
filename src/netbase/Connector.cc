#include <netbase/Connector.hpp>
#include <netbase/EventLoop.hpp>
#include <netbase/SocketsOps.hpp>
#include <netbase/Socket.hpp>
#include <assert.h>
#include <unistd.h>

#define MIN(A, B) ((A<B)?A:B)

namespace netbase 
{

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      backoff_(INIT_BACK_OFF),
      retryTimerId_(),
      // timeoutTimerId_(),
      state_(kDisconneted), // FIXME: ?
      connect_(false)
      // timeoutSecond_(0.0)
{
}

Connector::~Connector()
{
    assert(!channelPtr_);
    LOG(TRACE) << "Connector::~Connector[" << this << "]";
    // 不应该要手动cancel掉它们，如果Connector析构，
    // 应该要将它的生命期延伸到和timerid一样久远
    // 如果没有重传、也没有要求超时
    // ，则retryTimerId_是个默认值构造，其属性timerId是空值，不能cancel
    // if(!retryTimerId_.isInvalid())
    //     loop_->cancel(retryTimerId_);
    // if(!timeoutTimerId_.isInvalid())
    //     loop_->cancel(timeoutTimerId_);
}

// 该函数有两种可能被调用：
// 1. 旧有的描述符已经不能再用了，要把它close后然后生成新的
// 2. 连接成功后，描述符还绑着channel，这时要remove掉channel
//    但是之后的读写还是要通过这个描述符，所以这个描述符不能删
// 这就是为什么要返回sockfd的原因
int Connector::removeAndResetChannel()
{
    assert(channelPtr_);

    int sockfd = channelPtr_->fd();
    channelPtr_->disableAll();
    // remove the channel in pollfds
    channelPtr_->remove();
    // in channel handleRead, can not d'tor the channel directly
    // 此时调用channel->handleWrite, 如果直接析构掉channel，会发生什么则无法预估
    // 再具体点，如果此时连接上了，即发生了channelPtr_->handleWrite，然后
    // 在本函数直接调用channdlPtr_->remove()函数，这会是可怕的事情
    // 注意此处使用queueInLoop，排队进入Loop，也意味着在doPending 中执行
    // 此时所有channel对象都可以随意删除
    loop_->queueInLoop(std::bind(&Connector::resetChannel, shared_from_this()));

    return sockfd;
}

void Connector::resetChannel()
{
    // reset会调用析构函数
    channelPtr_.reset();
    // or ?
    // channelPtr_ = nullptr; 
}

void Connector::start()
{
    // if(state_ == kDisconneted)
    // {
        connect_ = true;
        loop_->runInLoop(std::bind(&Connector::startInLoop, this));
    // }
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();

    Connector::Connect();
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop() {
    loop_->assertInLoopThread();
    if(state_ == kConnecting) {
        state_ = kDisconneted;
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::restart()
{
    loop_->queueInLoop(std::bind(&Connector::restartInLoop, this));
}


void Connector::restartInLoop() {
    loop_->assertInLoopThread();
    state_ = kDisconneted;
    backoff_ = INIT_BACK_OFF;
    connect_ = true;
    startInLoop();
}

int Connector::Connect()
{
    int sockfd = wrap_sockets::createNonBlockingOrDie(serverAddr_.family());
    int ret = wrap_sockets::Connect(sockfd, serverAddr_);
    int savedErrno = ((ret == 0) ? 0 : errno);
    switch(savedErrno) 
    {
        {
        // -----无需理会的error
        // 连接立刻完成
        case 0:
        // 正在连接的错误码
        case EINPROGRESS:
        // 在阻塞式情况下，如果在三次握手过程被中断，则会返回EINTR。
        // 如果connect不会自动重启，再调用connect将会返回错误值EADDRINUSE
        case EINTR:
        // 连接已建立，再调用一次connect会失败，返回EISCONN
        case EISCONN:
            connecting(sockfd);
            break;
        }

        {
        // -----可以通过重试来尝试解决的error
        // 表明本机ephemeral port已经用完，要关闭socket再延期重试
        case EAGAIN:
        // 在阻塞式情况下，如果在三次握手过程被中断，则会返回EINTR。
        // 如果connect不会自动重启，再调用connect将会返回错误值EADDRINUSE
        case EADDRINUSE:
        // socket没有绑定地址，或者临时端口已经用完了
        case EADDRNOTAVAIL:
        // 对端连接拒绝
        case ECONNREFUSED:
        // 网络不可达
        case ENETUNREACH:
            LOG(ERROR) << "Connector::Connect error, try to retry";
            retry(sockfd);
            break;
        }

        {
        // ----无法通过重试来解决的error
        // 权限不够，要不就是没有socket file的写权限，要不就是没有父目录的搜索权限
        case EACCES:
        // 尝试通过没有广播标志位的socket连接广播地址
        case EPERM:
        // 地址没有正确的address family
        case EAFNOSUPPORT:
        // socket是非阻塞式时，而一个过去的connection attempt还没有结束
        case EALREADY:
        // socket不是一个有效的文件描述符
        case EBADF:
        // socket地址在用户的地址空间之外 ??
        case EFAULT:
        // 在非套接字上进行套接字操作
        case ENOTSOCK:
            LOG(ERROR) << "Connector::Connect error";
            wrap_sockets::Close(sockfd);
            break;
        }

        default:
            LOG(ERROR) << "Connector::Connect unexpected error";
            wrap_sockets::Close(sockfd);
            break;
    }
    return ret;
}

void Connector::connecting(int sockfd)
{
    // 只有到达这里才是kConnecting状态
    state_ = kConnecting;

    assert(!channelPtr_);
    channelPtr_.reset(new Channel(loop_, sockfd));
    channelPtr_->setWriteCallBack(std::bind(&Connector::handleWrite, this));
    channelPtr_->setErrorCallBack(std::bind(&Connector::handleError, this));

    channelPtr_->enableWriting();
}

void Connector::retry(int sockfd)
{
    // wrap_sockets::Close(sockfd);
    // 如果是在handleWrite中调用retry函数，则有可能在conneting过程就
    // 曾经被调用过stop函数，所以要对connect_状态判断一下
    // 注意此时一定要设置为kDisconnected状态
    state_ = kDisconneted;
    wrap_sockets::Close(sockfd);
    if(connect_) {
    // wrap_sockets::Close(sockfd);
        // 隔一定时间后再来重连
        // 要是runAfter后，本对象已经被析构了怎么办？答案是智能指针
        retryTimerId_ = loop_->runAfter(backoff_, 
                std::bind(&Connector::Connect, shared_from_this()));
        // retryTimerId_ = loop_->runAfter(backoff_,
        //         std::bind(&Connector::Connect, this));
        backoff_ = MIN(2 * backoff_, MAX_BACK_OFF);
    }
}

void Connector::handleWrite()
{
    assert(channelPtr_);
    if(state_ == kConnecting) {

        int savedErrno = 0;
        socklen_t len = sizeof(savedErrno);
        bool error_flag = false;

        int ret = getsockopt(channelPtr_->fd(), 
                SOL_SOCKET, SO_ERROR, &savedErrno, &len);
        if(ret == 0 && savedErrno !=0) 
        {
            // berkeley实现：getsockopt返回0，错误码放到savedErrno
            errno = savedErrno;
            error_flag = true;
        } else if (ret < 0){
            // solaris实现：getsockopt返回-1，错误码放到errno
            // must error, and errno has been set
            error_flag = true;
        }

        // removeAndResetChannel会remove掉channel
        int sockfd = removeAndResetChannel();
        if(error_flag)
        {
            LOG(ERROR) << "Connector::handleWrite connect error";
            // retry会close掉该sockfd, 且设置state_为kDisconneted
            retry(sockfd);

        } else if (wrap_sockets::isSelfConnect(sockfd)) {
            // 如果本地跟本地连，就有可能源ip源端口和目的ip目的端口是一样的
            // 如果出现这种情况，监听这个端口的服务器就不会启动
            // 解决办法是断开重连
            LOG(WARN) << "Connector::handleWrite - Self connect";
            // retry会close掉该sockfd, 且设置state_为kDisconneted
            retry(sockfd);
        } else {
            state_ = kConnected;

            if(connect_ && newConnectionCallback_)
            {
                // 创建前必须从pollfds中remove掉该channel，
                // 否则TcpConnection无法建立
                newConnectionCallback_(sockfd);
            }

        }
    }

}

/*
不打算做这个了
// Timerqueue 
void Connector::handleTimeout()
{
    loop_->assertInLoopThread();

    if(state_ != kConnecting)
        return;
    state_ = kTimeout;

    assert(channelPtr_);
    int sockfd = removeAndResetChannel();
    // assert(!channelPtr_);
    wrap_sockets::Close(sockfd);

    if(timeoutCallback_)
    {
        timeoutCallback_();
    }
}
*/

void Connector::handleError()
{
    // EINPROGRESS不是错，把它屏蔽掉吧
    if(errno != EINPROGRESS) 
        LOG(ERROR) << "Connector::handleError state=" << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = wrap_sockets::getSocketError(sockfd);
        LOG(TRACE) << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    } 
}

}
