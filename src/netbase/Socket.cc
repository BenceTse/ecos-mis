/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Socket.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: sockfd_的封装类，套接字类，主要用于服务器端 
* 历史记录: 无 
*/
#include <netbase/Socket.hpp>
#include <netbase/SocketsOps.hpp>
#include <netbase/InetAddress.hpp>
#include <logger/Logger.hpp>

// struct sockaddr_in
#include <netinet/in.h>
// bzero
#include <string.h>

namespace netbase
{
// 析构函数：关闭套接字
Socket::~Socket()
{
    wrap_sockets::Close(sockfd_);
}
// 获取tcp信息  
int Socket::fd() const
{
    return sockfd_;
}
// 把tcp信息格式化为字符串 
bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
    socklen_t len = sizeof(*tcpi);
    bzero(tcpi, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len) const
{
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok)
    {
        snprintf(buf, len, "unrecovered=%u "
            "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
            "lost=%u retrans=%u rtt=%u rttvar=%u "
            "sshthresh=%u cwnd=%u total_retrans=%u",
            tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
            tcpi.tcpi_rto,          // Retransmit timeout in usec
            tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
            tcpi.tcpi_snd_mss,
            tcpi.tcpi_rcv_mss,
            tcpi.tcpi_lost,         // Lost packets
            tcpi.tcpi_retrans,      // Retransmitted packets out
            tcpi.tcpi_rtt,          // Smoothed round trip time in usec
            tcpi.tcpi_rttvar,       // Medium deviation
            tcpi.tcpi_snd_ssthresh,
            tcpi.tcpi_snd_cwnd,
            tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    return ok;
}
// 绑定地址 
void Socket::bindAddress(const InetAddress& localaddr)
{
    const struct sockaddr* addr = localaddr.getSockAddr();
    wrap_sockets::Bind(sockfd_, addr, sizeof(struct sockaddr_in));
}
  
// 监听 
void Socket::listen()
{
    wrap_sockets::Listen(sockfd_);
}
  
// 接收连接  
int Socket::accept(InetAddress* peeraddr)
{
    // 准备一个空的addr
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t addrlen = sizeof(addr);

    // accept与fcntl之间，在多线程情况下可能存在竞态条件, 故采用Accept4
    int connfd = wrap_sockets::Accept4(sockfd_, 
            reinterpret_cast<struct sockaddr*>(&addr), 
            &addrlen, 
            SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0) {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}
  
// 关闭写端
void Socket::shutdownWrite()
{
    wrap_sockets::Shutdown(sockfd_, SHUT_WR);
}
// 关闭或开启地址复用 
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof optval));
}
    // FIXME CHECK
// 关闭或开启Nagle算法  
void Socket::setTcpNoDelay(bool on)
{
    // 设置套接字选项
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof optval));
}
//设置复用端口
void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG(ERROR) << "SO_REUSEPORT failed.";
    }
#else
    if (on)
    {
        LOG(ERROR) << "SO_REUSEPORT is not supported.";
    }
#endif
}
// 关闭或开启保活机制  
void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}


}
