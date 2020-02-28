/* 
* 版权声明: PKUSZ 216 
* 文件名称 : InetAddress.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: IPv4套接字sockaddr_in封装,网络地址类
* 历史记录: 无 
*/
#include <netbase/InetAddress.hpp>
#include <logger/Logger.hpp>

// for bzero
#include <string.h>
// inet_pton
#include <arpa/inet.h>
// snprintf
#include <stdio.h>
// for struct hostent 
#include <netdb.h>

#include <assert.h>

namespace netbase
{

namespace detail
{

// convert ipv4 or ipv6 address, from binary form to text form
const char* Inet_ntop(int family, const void* addrptr, char* strptr, size_t len)
{
    assert(strptr != nullptr);
    const char* ptr = nullptr;
    if((ptr = inet_ntop(family, addrptr, strptr, len)) == nullptr)
    {
        LOG(FATAL) << "inet_ntop error for " << strptr;
    }
    return ptr;
}

void Inet_pton(int family, const char* strptr, void* addrptr)
{
    int n;
    if((n = inet_pton(family, strptr, addrptr)) <= 0)
    {
        LOG(FATAL) << "inet_pton error for " << strptr;
    }
}

}

using namespace detail;
/* 
 * 地址构造 
 */  
InetAddress::InetAddress(uint16_t port, bool loopbackonly)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if(loopbackonly) {
        // 仅绑定回环地址，即值收到127.0.0.1上面的连接请求
        addr_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else {
        // 可以接收任意地址上连接请求
        addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    }
}
/* 
 * 地址构造 
 */ 
InetAddress::InetAddress(std::string ip, uint16_t port)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    Inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

std::string InetAddress::toIp() const
{
    char buf[32]; 
    const char* ip = Inet_ntop(addr_.sin_family, &addr_.sin_addr, buf, sizeof(buf));
    return ip;
}
// 返回ip和端口的字符串 
std::string InetAddress::toIpPort() const
{
    char buf[32];
    const char* ip = Inet_ntop(addr_.sin_family, &addr_.sin_addr, buf, sizeof(buf));
    snprintf(buf, sizeof(buf), "%c%s:%d", ip[0], ip + 1, toPort());
    return buf;
}
// 返回地址端口
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

uint32_t InetAddress::ipNetEndian() const
{
    return reinterpret_cast<uint32_t>(addr_.sin_addr.s_addr);
}

uint16_t InetAddress::portNetEndian() const
{
    return addr_.sin_port;
}

static __thread char t_resolveBuffer[64 * 1024];
bool InetAddress::resolve(const std::string& hostname, InetAddress* out)
{
	assert(out != nullptr);
	struct hostent hent;
	struct hostent* he = nullptr;
	int herrno = 0;
	bzero(&hent, sizeof(hent));

	int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
	if (ret == 0 && he != NULL)
	{
		assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	else
	{
		if (ret)
		{
			LOG(FATAL) << "InetAddress::resolve";
		}
		return false;
	}
}

}

