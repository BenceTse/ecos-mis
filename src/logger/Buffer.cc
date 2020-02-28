
#include <string.h>
#include "Buffer.hpp"

namespace Logger
{

//清空缓存
void Buffer::bzero()
{
    if (pbuf_ != nullptr)
        memset(pbuf_, 0, cap_);
}
//缓冲偏移量置为0
void Buffer::reset()
{
    off_ = 0;
}
//添加字符串到当前缓存后
void Buffer::append(const char *in, size_t len)
{
    if (pbuf_ != nullptr && avail() > len)
    {
        memcpy(pbuf_ + off_, in, len);
        off_ += len;
    }
}
//偏移量后移
void Buffer::addOff(size_t len)
{
    off_ += len;
}
//添加字符到当前缓存字符串后
void Buffer::append(char value)
{
    pbuf_[off_++] = value;
}

// ---------------------------------------------------------------------------------------------------------
//构造函数
HeapBuffer::HeapBuffer(size_t _cap)
{
    cap_ = _cap;
    off_ = 0;
    pbuf_ = new char[cap_];
}

HeapBuffer::~HeapBuffer()
{
    if (pbuf_ != nullptr)
    {
        delete[] pbuf_;
    }
}

HeapBuffer::HeapBuffer(HeapBuffer &&_buffer) noexcept
{
    cap_ = _buffer.cap_;
    off_ = _buffer.off_;
    pbuf_ = _buffer.pbuf_;

    _buffer.cap_ = _buffer.off_ = 0;
    _buffer.pbuf_ = nullptr;
}

HeapBuffer &HeapBuffer::operator=(HeapBuffer &&_buffer) noexcept
{
    if (this != &_buffer)
    {
        // free pbuf_
        if (pbuf_ != nullptr)
        {
            delete[] pbuf_;
        }
        cap_ = _buffer.cap_;
        off_ = _buffer.off_;
        pbuf_ = _buffer.pbuf_;

        // _buffer can be destructed
        _buffer.cap_ = _buffer.off_ = 0;
        _buffer.pbuf_ = nullptr;
    }
    return *this;
}

} // namespace Logger
