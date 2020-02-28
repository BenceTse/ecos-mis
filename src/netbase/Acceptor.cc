/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Accerptor.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 接收者Accpetor
* 历史记录: 无 
*/
#include <netbase/Acceptor.hpp>
#include <netbase/EventLoop.hpp>
#include <netbase/SocketsOps.hpp>

#include <netbase/InetAddress.hpp>

namespace netbase
{
/* 
 * 构造函数，创建套接字，并设置套接字选项，然后绑定地址 
 * 设置事件处理的读事件回调函数 
 */  
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenaddr)
    : loop_(loop),
      acceptSocket_(wrap_sockets::createNonBlockingOrDie(listenaddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false)
{
    // 地址复用
    acceptSocket_.setReuseAddr(true);
    // 绑定地址
    acceptSocket_.bindAddress(listenaddr);
    // 设置读事件的回调函数
    acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    // ::cloce(idleFd_);
}
// 监听
void Acceptor::listen()
{
    // ?
    loop_->assertInLoopThread();
    acceptSocket_.listen();
    // 启用事件处理器的读功能
    acceptChannel_.enableReading();
    listenning_ = true;
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    LOG(TRACE) << "acceptSocket = " << acceptSocket_.fd() << ", new connection";
    InetAddress peerAddr(0);
    // 接受一个连接
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0) {
        // 调用新连接到来回调函数 
        if(newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            wrap_sockets::Close(connfd); 
        }
    }
}

}
