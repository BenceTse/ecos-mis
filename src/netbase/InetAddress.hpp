/* 
* 版权声明: PKUSZ 216 
* 文件名称 : InetAddress.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: IPv4套接字sockaddr_in封装,网络地址类,只是sockaddr_in的一个包装，实现了各种针对sockaddr_in的操作 
* 历史记录: 无 
*/
#pragma once

#include <string>
#include <netinet/in.h>
#include <stdint.h>
// #include <inttypes.h>
// #include <unistd.h>

namespace netbase
{

class InetAddress
{
public:
    // 采用端口构造，通常用在tcpserver listening
    //loopbackonly==false可以接收任意地址的连接请求，true只能接收127.0.0.1请求
    explicit InetAddress(uint16_t port = 0, bool loopbackonly = false);

    // 采用端口和ip构造，ip形如"1.2.3.4"
    InetAddress(std::string ip, uint16_t port = 0);

    explicit InetAddress(const struct sockaddr_in& addr)
        : addr_(addr)
    {
    }
    //获取通信类型
    sa_family_t family() const { return addr_.sin_family; }
    // 主机字节序
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const struct sockaddr* getSockAddr() const { 
        // return static_cast<const struct sockaddr*>(&addr_);
        // return reinterpret_cast<const struct sockaddr*>(&addr_);
        return (struct sockaddr*) &addr_;
    }

    void setSockAddr(const struct sockaddr_in& addr) { addr_ = addr; }
    
    // 网络字节序
    uint32_t ipNetEndian() const;
    uint16_t portNetEndian() const;

    // 将hostname解析为ip address，不改变port和sin_family
    // return true on success
    // thread safe
    static bool resolve(const std::string& hostname, InetAddress* result);

private:
    struct sockaddr_in addr_;

};

}
