

#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>

#include <base/Thread.hpp>
#include "Buffer.hpp"
#include "LogSink.hpp"

namespace Logger
{
// 默认的buffer大小: 4M
const size_t LOG_BUF_BLOCK = 4 * 1024 * 1024;
// const size_t LOG_BUF_BLOCK = 4 * 1024;

const int flushInterval_ = 0.1 * 1000;
// const int flushInterval_ = 3 * 1000;

class AsynLogging
{
public:
    // // singleton，单例模式，懒汉模式
    static AsynLogging &instance()
    {
        // 从c++11开始，静态变量的初始化已经是线程安全了
        // 除此之外，可以采用std::call_once()
        // 又或者使用pthread_once()
        static AsynLogging asynlog;
        return asynlog;
    }

    // 禁止拷贝
    AsynLogging(const AsynLogging &) = delete;
    AsynLogging &operator=(const AsynLogging &) = delete;

    // 追加
    void append(const char *logline, int len);
    // 刷新缓冲区数据到输出设备上
    // void stopAndFlush();
    void stopAndFlush(bool willAbort);
    // 添加新的日志输出设备
    void addSink(detail::LogSink *sink);

    // private:
    //私有构造函数、析构函数
    AsynLogging();
    ~AsynLogging();

    void sinks_output(std::vector<std::unique_ptr<Buffer>> &bufferToWrite);
    void sinks_flush();
    void backendRun();
    // 允许有多个sinker
    void addSink(std::unique_ptr<detail::LogSink> &&);
    void addDefaultSink();

    std::mutex mutex_;
    std::condition_variable condition_;

    // 双缓冲技术：A满则与B交换，继续往A写，打印B
    std::unique_ptr<Buffer> currentBuffer_;
    std::unique_ptr<Buffer> nextBuffer_;
    // 如果日志生成太快，则构造可变长的缓冲区
    std::vector<std::unique_ptr<Buffer>> buffers_;

    // 后端线程一次性输出所有生成的日志数据
    std::unique_ptr<Buffer> newBuffer1;
    std::unique_ptr<Buffer> newBuffer2;
    std::vector<std::unique_ptr<Buffer>> bufferToWrite;

    std::vector<std::unique_ptr<detail::LogSink>> sinkVec_;
    bool running_;
    // isEnter用来标志是否是已近进入了backendRun的while循环
    bool isEnter;
    std::mutex isEnter_mutex_;
    std::condition_variable isEnter_condition_;
    //后台守护进程
    Concurrency::Thread backThread_;
};

} // namespace Logger
