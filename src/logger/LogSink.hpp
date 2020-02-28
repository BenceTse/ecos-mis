
#pragma once
#include "Buffer.hpp"
#include <vector>
#include <memory>

namespace Logger
{

namespace detail
{

class LogSink
{
public:
    // LogSink必须实现operator()，定制如何打印日志信息
    virtual int operator()(std::vector<std::unique_ptr<Buffer>> &bufferVec) = 0;
    // 刷新输出缓冲区
    virtual void flush() = 0;
};

} // namespace detail
} // namespace Logger
