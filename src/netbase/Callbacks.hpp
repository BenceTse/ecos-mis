/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Callbacks.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: TCP连接相关回调函数定义
* 历史记录: 无 
*/
#pragma once

#include <functional>
// shared_ptr
#include <memory>

namespace netbase
{
// TcpConnection表示一个TCP的连接
class TcpConnection;
class Buffer;
class Timestamp;
class EventLoop;
// 定义TcpConnection的指针类型
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
// 计时器回调函数的定义  
typedef std::function<void()> TimerCallback;
// 连接完成回调函数的定义 
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
// 关闭一个连接的回调函数 
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
// typedef std::function<void (const TcpConnectionPtr&, char*, size_t)> MessageCallback;
// 当数据到来的时候的回调函数，此时数据已经被存放在buffer中 
typedef std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;
// 写完成回调函数的定义
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
// 高水位标记回调函数的定义
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

typedef std::function<void(EventLoop*)> ThreadInitCallback;

// Connector
typedef std::function<void()> TimeoutCallback;

}
