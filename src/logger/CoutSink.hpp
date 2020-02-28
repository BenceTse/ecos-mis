
#pragma once

#include "LogSink.hpp"
#include "stdio.h"
#include <iostream>

namespace Logger
{

namespace detail
{

class CoutSink : public LogSink
{
public:
    //重载（),将缓冲区BUFFE打印到标准输出
    int operator()(std::vector<std::unique_ptr<Buffer>> &bufferVec) override
    {
        for (auto it = bufferVec.begin();
             it != bufferVec.end();
             ++it)
        {
            const char *pbuf = (*it)->c_buf();
            size_t size = (*it)->size();
            // std::cout << "size(): " << size << std::endl;
            fwrite(pbuf, sizeof(char), size, stdout);
        }

        return 0;
    }
    //刷新标准输出到显示器
    void flush() override
    {
        fflush(stdout);
    }
};

} // namespace detail
} // namespace Logger
