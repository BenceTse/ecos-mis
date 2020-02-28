
#pragma once

#include <cstddef>
// #include <assert.h>
#include <string.h>
#include "Buffer.hpp"

#include <base/utils.hpp>

// 打印整型类型的数据，如int、long、string等
#define IntergerToStream(TYPE)        \
    LogStream &operator<<(TYPE value) \
    {                                 \
        formatInteger(value);         \
        return *this;                 \
    }

// #include <iostream>
// using namespace std;
namespace Logger
{

const int MAX_ENTRY_SIZE = 4 * 1024;
const int MAX_NUMBER_BYTE = 100;

/*
   *    根据不同的类型完成数据的格式化输出
   *    确保类型安全
   */

class LogStream
{
public:
    typedef StackBuffer<MAX_ENTRY_SIZE> EntryBuffer;

    LogStream() = default;

    LogStream(const LogStream &) = delete;
    LogStream &operator=(const LogStream &) = delete;

    EntryBuffer &getEntry()
    {
        return entry_;
    }
    //将T类型数据转换为字符串类型添加到EntryBuffer尾部
    template <typename T>
    void formatInteger(T value)
    {
        size_t len = utils::convert(entry_.c_buf() + entry_.off(), value);
        entry_.addOff(len);
    }

    LogStream &operator<<(char value);
    LogStream &operator<<(unsigned char value);

    // Integer to LogStream
    // 整型类型
    IntergerToStream(short);
    IntergerToStream(unsigned short);
    IntergerToStream(int);
    IntergerToStream(unsigned int);
    IntergerToStream(long);
    IntergerToStream(unsigned long);
    IntergerToStream(long long);
    IntergerToStream(unsigned long long);

    // string
    // 字符串类型
    LogStream &operator<<(const char *);
    LogStream &operator<<(const unsigned char *);
    LogStream &operator<<(const std::string &);

    // 浮点数类型
    LogStream &operator<<(float);
    LogStream &operator<<(double);

    // 指针类型
    LogStream &operator<<(void *p);
    LogStream &operator<<(const void *p);

private:
    EntryBuffer entry_;
};

} // namespace Logger
