/* 
* 版权声明: PKUSZ 216 
* 文件名称 : SocketsOps.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 套接字操作类，对各种套接字的操作进行封装，基本都是在原来的操作基础上加上一些错误处理和日志记录
* 历史记录: 无 
*/
// write read close fork
#include <unistd.h>
// struct sockaddr_in
#include <netinet/in.h>

#include <netbase/Socket.hpp>
#include <netbase/InetAddress.hpp>
#include <logger/Logger.hpp>

namespace netbase
{
namespace wrap_sockets
{

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr)
{
  return static_cast<const struct sockaddr*>((const void*)(addr));
}

// 关闭套接字  
void Close(int sockfd)
{
    if(::close(sockfd) < 0)
    {
        LOG(FATAL) << "wrap_sockets::close error";
    }
}
// 绑定地址  
void Bind(int fd, const struct sockaddr* sa, socklen_t salen)
{
    if(::bind(fd, sa, salen) < 0)
    {
        LOG(FATAL) << "wrap_sockets::Bind error";
    }
}

// void Listen(int fd, int backlog)
// 监听 
void Listen(int fd, int backlog = SOMAXCONN)
{
    if(::listen(fd, backlog) < 0)
    {
        LOG(FATAL) << "wrap_sockets::Listen error";
    }
}
// 接收连接
int Accept4(int fd, struct sockaddr *sa, socklen_t *salenptr, int flags)
{
    int connfd;
    if((connfd = ::accept4(fd, sa, salenptr, flags)) < 0)
    {
        int savedErrno = errno;
        LOG(ERROR) << "wrap_sockets::Accept4";
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG(FATAL) << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG(FATAL) << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    return connfd;
}

int Socket(int family, int type, int protocol)
{
    int fd;
    if((fd = ::socket(family, type, protocol)) < 0)
    {
        LOG(FATAL) << "wrap_sockets::Socket error";
    }
    return fd;
}

void Shutdown(int fd, int flag)
{
    if(::shutdown(fd, flag) < 0)
    {
        LOG(FATAL) << "wrap_socket::Shutdown error";
    }
}
// 连接服务器 
int Connect(const netbase::Socket& sock, const netbase::InetAddress& addr)
{
    return ::connect(sock.fd(), addr.getSockAddr(), sizeof(struct sockaddr_in));
}
// 连接服务器 
int Connect(int sockfd, const netbase::InetAddress& addr)
{
    return ::connect(sockfd, addr.getSockAddr(), sizeof(struct sockaddr_in));
}

// 写数据 
ssize_t Write(int fd, const char* buf, size_t len)
{
    return ::write(fd, buf, len);
    // if(::write(fd, buf, len) != len)
    // {
    //     LOG(FATAL) << "wrap_socket::Write error";
    // }
}
// 创建非阻塞的套接字  
int createNonBlockingOrDie(sa_family_t family)
{
    // valgrind
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG(FATAL) << "wrap_sockets::createNonBlockingOrDie error";
    }
    return sockfd;
}
// 获取本地地址  
/*
struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in localAddr; 
    bzero(&localAddr, sizeof(localAddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localAddr));
    if(::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localAddr), &addrlen) < 0)
    {
        LOG(FATAL) << "wrap_sockets::getLocalAddr error";
    }
    return localAddr;
}
*/

int getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in localAddr; 
    bzero(&localAddr, sizeof(localAddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localAddr));
    if(::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localAddr), &addrlen) < 0)
    {
        LOG(FATAL) << "wrap_sockets::getLocalAddr error";
    }
    return localAddr;
}

struct sockaddr_in6 getLocalAddr_ipv6(int sockfd)
{
  struct sockaddr_in6 localaddr;
  bzero(&localaddr, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, (struct sockaddr*)(&localaddr), &addrlen) < 0)
  {
    LOG(FATAL) << "sockets::getLocalAddr";
  }
  return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd)
{
  struct sockaddr_in6 peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  // if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
  if (::getpeername(sockfd, (struct sockaddr*)(&peeraddr), &addrlen) < 0)
  {
    LOG(FATAL) << "sockets::getPeerAddr";
  }
  return peeraddr;
}

bool isSelfConnect(int sockfd)
{
  struct sockaddr_in6 localaddr = getLocalAddr_ipv6(sockfd);
  struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET)
  {
    const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
    const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port
        && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  }
  else if (localaddr.sin6_family == AF_INET6)
  {
    return localaddr.sin6_port == peeraddr.sin6_port
        && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
  }
  else
  {
    return false;
  }
}

}
}
