/* 
* 版权声明: PKUSZ 216 
* 文件名称 : SocketsOps.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 套接字操作类，对各种套接字的操作进行封装，基本都是在原来的操作基础上加上一些错误处理和日志记录
* 历史记录: 无 
*/
#pragma once

#include <sys/socket.h>

#include <netbase/Socket.hpp>
#include <netbase/InetAddress.hpp>

namespace netbase
{
namespace wrap_sockets
{

// class netbase::Socket;
// class netbase::InetAddress;
// 关闭套接字  
void Close(int sockfd);
// 绑定地址  
void Bind(int fd, const struct sockaddr* sa, socklen_t salen);
// 监听 
void Listen(int fd, int backlog = SOMAXCONN);
// 接受连接 
int Accept4(int fd, struct sockaddr *sa, socklen_t *salenptr, int flags);

int Socket(int family, int type, int protocol);
// 把套接字的写端关闭  
void Shutdown(int fd, int flag);

// cliSock 设置为非阻塞套接字，调用connect将立即返回一个EINPROGRESS错误，
// 不过已经发起的TCP三路握手继续进行，
// 正常来说应该要使用select/poll来检测连接成功或失败
int Connect(const netbase::Socket& sock, const netbase::InetAddress& addr);
int Connect(int sockfd, const netbase::InetAddress& addr);

// 对于非阻塞TCP，如果发送缓冲区没有空间，则立即返回EWOULDBLOCK错误
// 如果发送缓冲区有一些空间，则返回值将是能够复制到该缓存区中的字节数
ssize_t Write(int fd, const char* buf, size_t len);

// AF_LOCAL or AF_INET ?
int createNonBlockingOrDie(sa_family_t family);

struct sockaddr_in getLocalAddr(int sockfd);

bool isSelfConnect(int sockfd);

int getSocketError(int sockfd);

}
}
