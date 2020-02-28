
#pragma once
#include <fstream>

#include "LogSink.hpp"
#include "stdio.h"

namespace Logger
{

namespace detail
{
class FileSink : public LogSink
{
public:
    //构造函数，若文件不存在则创建文件
    FileSink(const char *path) : out(fopen(path, "a")) {}
    //析构函数，关闭文件描述符
    ~FileSink() { fclose(out); }
    //重载（)，将缓冲区字符串写入指定文件
    int operator()(std::vector<std::unique_ptr<Buffer>> &bufferVec) override
    {
        for (auto it = bufferVec.begin();
             it != bufferVec.end();
             ++it)
        {
            const char *pbuf = (*it)->c_buf();
            size_t sz = (*it)->size();
            // out << pbuf;
            fwrite(pbuf, sizeof(char), sz, out);
        }
        return 0;
    }
    //刷新缓冲到文件中
    void flush() override
    {
        fflush(out);
        // out.flush();
    }

private:
    // std::ofstream out;
    FILE *out;
};

} // namespace detail

} // namespace Logger
