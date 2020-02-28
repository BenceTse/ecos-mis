/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Connector.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 连接者/连接器
* 历史记录: 无 
*/
/*
非阻塞connect：当连接成功建立，描述符可写；当连接建立错误时，描述符变为既可读又可写
Connector的工作流程如下：
1、根据构造函数传递进来的EventLoop和服务器地址等信息进行初始化
2、然后调用start开始连接
3、start内部创建一个非阻塞的套接字，然后向服务器发起连接
4、由于是非阻塞的，connect函数会马上返回，并返回一些错误码
5、如果服务器已经开启、网络设置没有问题、地址没有问题的话，connect函数通常会返回一个错误码表示套接字正在连接；此时我们创建一个事件处理器，并设置写事件的回调函数、错误事件的回调函数，然后等待连接的完成，连接建立完成会触发写事件，然后我们在里面调用连接建立完成回调函数
6、如果网络阻塞，那么连接可能超时，一旦超时我们就等待指定的重连时间，然后移除原来的事件处理器，创建一个新套接字和新的的事件处理器，然后按照步骤5进行重连
 */
#pragma once

#include <netbase/InetAddress.hpp>
#include <netbase/Channel.hpp>
#include <netbase/Timer.hpp>

#include <functional>
#include <memory>

namespace netbase 
{

class EventLoop;

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;
    typedef std::function<void()> TimeoutCallback;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();
    // 设置新链接到来的回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb) { 
        newConnectionCallback_ = cb; 
    }

    // 不能设置比默认更长的timeout( 大致75s ?)
    // void setTimeoutCallback(double timeoutSecond, const TimeoutCallback& cb) {
    //     timeoutSecond_ = timeoutSecond;
    //     timeoutCallback_ = cb;
    // }
    // 开始启动连接
    void start();
    // 重新连接
    void restart();
    // 停止连接
    void stop();
    bool isConnected() { return state_ == kConnected; }
    bool isConnecting() { return state_ == kConnecting; }

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

private:
    // FIXME: should be larger
    constexpr static double MAX_BACK_OFF = 4.0;
    constexpr static double INIT_BACK_OFF = 0.5;
    // 所谓kConnecting：调用了connect函数后，因为非阻塞，会直接返回
    //     此时会等待可写事件发生，这段状态才是kConnecting
    enum StateE { kDisconneted, kConnecting, kConnected};

    EventLoop* loop_;
    InetAddress serverAddr_;
    double backoff_;
    TimerId retryTimerId_;
    // TimerId timeoutTimerId_;
    StateE state_;
    bool connect_;
    NewConnectionCallback newConnectionCallback_;
    // double timeoutSecond_;
    TimeoutCallback timeoutCallback_;
    // 事件处理器
    std::unique_ptr<Channel> channelPtr_;
    // 移除并重置事件处理器 
    int removeAndResetChannel();
    // 重置事件处理器
    void resetChannel(); 
    void createChannel();
    void startInLoop();
    void restartInLoop();
    void stopInLoop();
    int Connect();
    void connecting(int sockfd);
    void retry(int sockfd);
    // void retrying();
    void handleWrite();
    // 处理错误 
    void handleError();
    void handleTimeout();
};

}
