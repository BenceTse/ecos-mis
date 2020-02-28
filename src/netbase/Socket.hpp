/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Socket.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: sockfd_的封装类，套接字类，主要用于服务器端 
* 历史记录: 无 
*/
#pragma once
#include <netinet/tcp.h>

namespace netbase
{

class InetAddress;

class Socket
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }
    
    // Socket(Socket&&)
    ~Socket();
    //返回socket句柄
    int fd() const;

    // return true if success
    bool getTcpInfo(struct tcp_info*) const;
    bool getTcpInfoString(char* buf, int len) const;

    // abort if address in use
    void bindAddress(const InetAddress& localaddr);
    // abort if address in use
    void listen();

    // 如果成功，则返回对应的非负的文件描述符，且*peeraddr已经被赋值
    //           且已经被设置为 NON_BLOCKING | SOCK_CLOEXEC
    //           SOCK_CLOEXEC: 进程使用fork()加上execve()的时候自动关闭打开的文件描述符, 
    //                         避免该描述符泄露给子进程
    // 如果失败，则返回-1，*peeraddr保持原样
    int accept(InetAddress* peeraddr);

    void shutdownWrite();

    // 启动TCP_NODELAY(禁用了Negle算法)，允许小包发送，可以降低时延
    // 关闭TCP_NODELAY(启用了Negle算法), 缓存一定量的数据，封成一个大包一次性发送，提高了带宽利用率，提高了时延
    void setTcpNoDelay(bool on);

    // enable/disable SO_REUSEADDR
    void setReuseAddr(bool on);

    // enable/disable SO_REUSEPORT
    // 允许多个进程或者线程绑定到同一端口，提高服务器程序的性能
    void setReusePort(bool on);

    // Enable/disable SO_KEEPALIVE
    void setKeepAlive(bool on);

private:
    const int sockfd_;

};


}
