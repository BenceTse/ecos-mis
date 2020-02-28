
#include "LogStream.hpp"
#include <sstream>

namespace Logger
{

// 重载<<操作符
LogStream &LogStream::operator<<(char value)
{
    // 实际调用logger的append方法
    entry_.append(value);
    return *this;
}

LogStream &LogStream::operator<<(unsigned char value)
{
    // entry_.append(reinterpret_cast<char>(value));
    entry_.append(value);
    return *this;
}

LogStream &LogStream::operator<<(const char *str)
{
    if (str)
    {
        size_t len = strlen(str);
        entry_.append(str, len);
    }
    else
    {
        entry_.append("(null)", 6);
    }

    return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str)
{
    return operator<<(reinterpret_cast<const char *>(str));
}

LogStream &LogStream::operator<<(const std::string &str)
{
    entry_.append(str.c_str(), str.size());
    return *this;
}

LogStream &LogStream::operator<<(float f)
{
    return operator<<(static_cast<double>(f));
}

LogStream &LogStream::operator<<(double d)
{
    char buf[MAX_NUMBER_BYTE];
    size_t len = snprintf(buf, MAX_ENTRY_SIZE, "%.12g", d);
    entry_.append(buf, len);
    return *this;
}

LogStream &LogStream::operator<<(void *p)
{
    // 0xXXXXXX
    // uintptr_t v = reinterpret_cast<uintptr_t>(p);
    // std::stringstream ss;
    // ss << "0x" << std::hex << v;
    // if(entry_.avail() > ss.str().size())
    // {
    //     entry_.append(ss.str().c_str(), ss.str().size());
    // }
    return operator<<(static_cast<const void *>(p));
    // return *this;
}

LogStream &LogStream::operator<<(const void *p)
{
    // 0xXXXXXX
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    std::stringstream ss;
    ss << "0x" << std::hex << v;
    if (entry_.avail() > ss.str().size())
    {
        entry_.append(ss.str().c_str(), ss.str().size());
    }
    return *this;
}

} // namespace Logger
