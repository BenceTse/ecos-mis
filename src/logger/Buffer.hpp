
#pragma once

// #include <cstdint>
#include <cstddef>
#include <type_traits>

namespace Logger
{

class Buffer
{
public:
    Buffer() : cap_(0), off_(0), pbuf_(nullptr){};
    //未实现
    Buffer(size_t);

    //禁止复制构造和=号重载
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    //在当前buffer后接长度为len的in字符串
    void append(const char *in, size_t len);

    //如果buffer不为空，将buffer置为0
    void bzero();
    //重置buffer
    void reset();

    //获取buffer最大容量
    inline size_t capacity() const
    {
        return cap_;
    }

    //获取buffer剩余可用长度
    inline size_t avail() const
    {
        return cap_ - off_;
    }

    // 获取buffer的指针
    inline char *c_buf()
    {
        return pbuf_;
    }

    // 获取追加点的指针
    inline char *data()
    {
        return pbuf_ + off_;
    }
    //获取buffer长度
    inline size_t off() const
    {
        return off_;
    }
    //获取buffer长度
    inline size_t size() const
    {
        return off_;
    }
    //将buffer长度增加，off_t未做边界处理
    void addOff(size_t len);
    //在buffer后添加字符，off_未做边界处理
    void append(char value);

protected:
    size_t cap_; //buffer最大长度
    size_t off_; //buffer当前长度
    char *pbuf_; //buffer
};

// 堆buffer，该类型buffer存放在内存堆中
class HeapBuffer : public Buffer
{
public:
    HeapBuffer(size_t _cap);
    ~HeapBuffer();

    HeapBuffer(HeapBuffer &&) noexcept;
    HeapBuffer &operator=(HeapBuffer &&) noexcept;
};

// 栈buffer，该类型的buffer存放在栈中
template <size_t CAP>
class StackBuffer : public Buffer
{
public:
    StackBuffer()
    {
        static_assert(CAP > 0, "The capacity of buffer must be positive.");
        cap_ = CAP;
        off_ = 0;
        pbuf_ = innerbuf;
    }

private:
    char innerbuf[CAP];
};

} // namespace Logger
