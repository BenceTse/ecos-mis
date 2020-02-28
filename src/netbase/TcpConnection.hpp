/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TcpConnection.hpp
* 创建日期: 2018/03/04 
* 文件描述: TCP客户端与TCP服务器之间的一个连接
* 历史记录: 无 
*/
// TcpConnection并不直接提供接口给用户使用，而是进一步封装在TcpServer/TcpClient
// TcpConnection采用shared_ptr来管理，原因在于防止串话，如：
//     多线程情况，有可能write一个刚刚被close的并open的fd，则发生了串话
// 关注四个事件：链接建立(connector)、链接断开、有数据读、数据发送完毕
//
/* 
TCP客户端与TCP服务器之间的一个连接 
客户端和服务器都使用这个类 
对于服务器来说，每当一个客户到来的时候就建立一个TcpConnection对象 
对于客户端来说，一个TcpConnection对象就表示客户端与服务器的一个连接 
用户不应该直接使用这个类  
TCP客户端与TCP服务器之间的一个连接用TcpConnection表示。
对于服务器来说，每当一个客户到来的时候就建立一个TcpConnection对象
对于客户端来说，一个TcpConnection对象就表示客户端与服务器的一个连接
用户不应该直接使用这个类.
从服务器的角度来说：
1、服务器使用Acceptor创建套接字然后绑定、监听
2、Reactor处理Acceptor的读事件，Acceptor的读事件就是有一个新连接到来。
3、Acceptor的读事件处理函数是服务器的newConnection函数，这个函数建立一个TcpConnection对象，然后调用用户自定义的的连接建立回调函数，把这个TcpConnection对象传给用户。
4、用户得到TcpConnection对象之后，可以使用这个对象进行数据发送、主动关闭连接等操作。
 */ 
#pragma once

#include <string>

#include <netbase/Socket.hpp>
#include <netbase/Channel.hpp>
#include <netbase/InetAddress.hpp>
#include <netbase/Callbacks.hpp>
#include <netbase/Buffer.hpp>

namespace netbase
{

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& name,
                  int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();
    // 判断是否已经连接
    bool connected() const { return state_ == kConnected; }
    // 获取名字
    std::string name() const { return name_; }
    // 对端的地址
    InetAddress peerAddress() const { return peerAddr_; }
    InetAddress localAddress() const { return localAddr_; }

    // 连接完成，当tcp服务器接收到一个新的连接时调用它
    void connectEstablished();
    // 连接已经被销毁，当一个连接被服务器移除的时候
    void connectDestroyed();
    // 设置连接回调函数
    void setConnectionCallback(const ConnectionCallback& cb) { 
        connectionCallback_ = cb; 
    }
    // 设置数据到来的回调函数
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    // 设置关闭的回调函数 
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
    // 设置写完成的回调函数
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) 
    {
        writeCompleteCallback_ = cb; 
    }
    // 设置到达高水位标记的回调函数
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb)
    {
        highWaterMarkCallback_ = cb; 
    }

    void setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }
    // 使用tcp保活
    void setKeepAlive(bool on) { socket_->setKeepAlive(on); }
    // 获取所属的Reactor
    EventLoop* getLoop() const { return loop_; }

    // 采用移动语义消除构造可调用对象是的拷贝开销
    void send(Buffer* buf);
    // Thread safe
    // 发送数据  
    void send(const std::string& message);
    void send(std::string&& message);
    // Thread safe
    // 关闭连接
    void shutdown();
    // 主动断开连接
    void forceClose();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

private:
    enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected};

    // Buffer并不是线程安全的，但是inputBuffer_都在onMessage中使用，
    // 而onMessage必然在IO thread中运行
    // 输入缓冲区
    Buffer inputBuffer_;
    // send操作会将outputBuffer_的数据发送出去，尽管send会被多个线程调用
    // 但是内部会通过sendInLoop方法，将实际对outputBuffer_的操作转给IO线程
    // 输出缓冲区 
    Buffer outputBuffer_;

    void setState(StateE s) { state_ = s; }
    // 当有数据可读则触发该handleRead，然后回调messageCallback。
    // 对于一次messageCallback，只会触发一次read，并没有反复read直到返回EAGAIN
    // 原因在于：对于追求低延迟的程序来说，这样做事高效的
    // 其次，照顾了公平性，不会因为某个连接上的数据量过大而影响其他连接处理消息
    // 处理读
    void handleRead(Timestamp receiveTime);
    // 处理关闭
    void handleClose();
    // 处理错误
    void handleError();
    // 处理写
    void handleWrite();

    // void sendInLoop(const Buffer& buf);
    // 不但会发送所有的数据，而且会清空buf的内容
    // FIXME: 至少需要一次拷贝(从buf到output buffer)，更效率的做法是：交换两个buf
    void sendInLoop(Buffer& buf);
    // 发送消息
    void sendInLoop(const std::string& message);
    void sendInLoop(const void* ptr, size_t len);
    // 在循环中关闭连接
    void shutdownInLoop();
    void forceCloseInLoop();

    EventLoop* const loop_;
    // 名字 
    std::string name_;
    // 状态 
    StateE state_;

    //接受**已经建立**好连接的socket fd
    // 注意socket_, channel_生命期与所在对象一样长
    // socket_析构的时候会调用close，来关掉打开的描述符
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    // 本地地址
    InetAddress localAddr_;
     // 远端地址
    InetAddress peerAddr_;
    // 连接回调函数/断开链接回调函数 
    ConnectionCallback connectionCallback_;
    // 数据到来回调函数
    MessageCallback messageCallback_;
    // 关闭连接的回调函数
    CloseCallback closeCallback_;
    // 写完成回调函数
    WriteCompleteCallback writeCompleteCallback_; 
    // 当输出缓冲区到达预设的阈值，则会调用该函数
    HighWaterMarkCallback highWaterMarkCallback_;
    // 高水位标记
    size_t highWaterMark_;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
}
