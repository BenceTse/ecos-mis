/* 
* 版权声明: PKUSZ 216 
* 文件名称 : TClient.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: Tcp客户端封装，构造时必须指定所在的EventLoop
* 历史记录: 无 
*/
// TcpClient的工作流程如下：
// 1、根据构造函数传递进来的EventLoop和服务器地址等信息进行初始化
// 2、建立一个连接器Connector
// 3、设置连接器的事件处理器的写事件回调函数为newConnection（当连接建立完成的时候会触发写事件，Connector的事件处理进行处理，并调用写事件的回调函数）
// 4、调用connect向对服务器进行连接，内部调用连接器的start函数，然后等待连接完成
// 5、连接建立完成的时候会触发写事件，Connector的事件处理进行处理，并调用写事件的回调函数ewConnection
//     5.1、设置新链接的id和名字
//     5.2、创建一个TcpConnection对象
//     5.3、设置TcpConnection对象的回调函数：用户的连接建立回调函数、用户的数据到来回调函数、用户的写完成回调函数
//     5.4、调用TcpConnection的connectEstablished函数表示连接建立完毕
#pragma once
#include <mutex>

#include <netbase/Connector.hpp>
#include <netbase/InetAddress.hpp>
#include <netbase/TcpConnection.hpp>

#include <assert.h>

namespace netbase
{

class EventLoop;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient
{
public:
    TcpClient(EventLoop* loop,
              const InetAddress& servAddr,
              const std::string& nameArg);
    ~TcpClient();

    // 尝试对指定的server发起连接
    void connect();
    // 断开连接，如果未发送/接受完的数据，会在真正断开前自动完成
    void disconnect();
    // 重新发起连接
    void reconnect();
    // 停止连接
    void stop();

    TcpConnectionPtr connection() {
        std::lock_guard<std::mutex> lock(mutex_);
         // assert(connection_ != nullptr);
        return connection_;
    }

    void send(const std::string& msg) {
        if(connection_ && connection_->connected()) 
            connection_->send(msg);
    }

    // 设置连接完成的回调函数
    void setConnectionCallback(const ConnectionCallback& cb) { 
        connectionCallback_ = cb; 
    }
    // 设置数据到来的回调函数 
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    // 设置写完成的回调函数
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) 
    { 
        writeCompleteCallback_ = cb; 
    }
    // 设置超时的回调函数
    // void setTimeoutCallback(double timeoutSecond, const TimeoutCallback& cb) {
    //     connector_->setTimeoutCallback(timeoutSecond, cb);
    // }

    // non-copyable
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    bool isConnectedT(){
        if(connection_ && connection_->connected())
            return true;
        return false;
    }

private:
    // 所属的Reactor
    EventLoop* loop_;
    // 服务器地址
    InetAddress servAddr_;
    // Client名字
    std::string name_;
    // 要用shared_ptr管理连接器，因为它的生命周期可能会比TcpClient长
    std::shared_ptr<Connector> connector_;
    // 客户端到服务器的连接 
    TcpConnectionPtr connection_;
    // 保护connection_
    std::mutex mutex_;
    bool isConnected;
    // std::
    // 连接建立完成的回调函数
    ConnectionCallback connectionCallback_;
    // 删除连接 
    // void removeConn(const TcpConnectionPtr& conn);
    void removeConnection(const TcpConnectionPtr& conn);
    // 数据到来回调函数
    MessageCallback messageCallback_;
    // 写完成回调函数
    WriteCompleteCallback writeCompleteCallback_;
    // TimeoutCallback timeoutCallback_;
    // 连接建立完毕会调用这个函数
    void newConnectionCallback(int sockfd);

};

}
