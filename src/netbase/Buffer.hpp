/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Buffer.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 动态增长的数组
* 历史记录: 无 
*/
//
//  线程不安全，安全等级和std::vector一样
//  Buffer内部结构形如下：
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
//
#pragma once

#include <vector>
#include <stddef.h>
// #include <assert.h>
#include <algorithm>

#include <logger/Logger.hpp>

namespace netbase
{

class Buffer
{

public:
    static const size_t kCheapPrepend = 8;
     //buffer默认初始化大小1kB
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initSize = kInitialSize) :
        buffer_(kCheapPrepend + initSize),
        readIndex(kCheapPrepend),
        writeIndex(kCheapPrepend)
    {
        // LOG(DEBUG) << buffer_.size() << " " << buffer_.capacity();
    }

    // 采用移动语义将rv的资源全部转交给自己
    Buffer(Buffer&& rv) :
        buffer_(std::move(rv.buffer_)),
        readIndex(rv.readIndex),
        writeIndex(rv.writeIndex)
    {
        // LOG(DEBUG) << "Buffer move c'tor";
    }

    Buffer(const Buffer& buf) :
        buffer_(buf.buffer_),
        readIndex(buf.readIndex),
        writeIndex(buf.writeIndex)
    {
        LOG(DEBUG) << "***************Buffer copy c'tor-----------------";
    }
    //从文件描述符fd中读数据
    ssize_t readFd(int fd, int* savedErrno);
    //可读字节
    size_t readableBytes() const { return writeIndex - readIndex; }
    // size_t writableBytes() const { return buffer_.size() - readIndex; }
    size_t writableBytes() const { return buffer_.size() - writeIndex; }

    void prepend(const void* ptr, size_t len) {
        CHECK(readIndex >= len) << "pre available space not enough";
        // assert(readIndex >= len);
        readIndex -= len;
        // 模板推导必须两个迭代器是一样的
        // std::copy((const char*)ptr, (const char*)ptr + len, begin());
        const char* s = static_cast<const char*>(ptr);
        std::copy(s, s+len, begin()+readIndex);
    }

    void append(const void* ptr, size_t n)
    {
        if(writableBytes() < n && n <= (buffer_.size() - readableBytes() - kCheapPrepend)) {
            // 如果readindex前面的空间加上writeIndex后面的空间，不比n小，
            // 则将数据拷贝到前面，腾出空间
            std::copy(&buffer_[readIndex], &buffer_[writeIndex], &buffer_[kCheapPrepend]);
            writeIndex = writeIndex - ( readIndex - kCheapPrepend);
            readIndex = kCheapPrepend;
        }
        // buffer_.insert(buffer_.begin() + writeIndex, (const char*)ptr, (const char*)ptr + n);
        buffer_.insert(buffer_.begin() + writeIndex, 
                       static_cast<const char*>(ptr), 
                       static_cast<const char*>(ptr) + n);
        writeIndex += n;
        // printState();
    }
    //返回并重置buffer
    std::string retrieveAsString()
    {
        const char* str = peek();
        size_t len = readableBytes();
        retrieveAll();
        return std::string(str, len);
    }

    void* writeBegin() {
        return &*buffer_.begin() + writeIndex;
    }

    void hasWriten(size_t len) {
        writeIndex += len;
    }
    //重置len长度的buffer
    void retrieve(size_t len)
    {
        CHECK(len <= readableBytes()) << "len <= readableBytes()";
        // assert(len <= readableBytes());
        if(len == readableBytes())
            retrieveAll();
        else
            readIndex += len;
    }
    //重置buffer
    void retrieveAll()
    {
        readIndex = writeIndex = kCheapPrepend;
    }

    char* peek() {return &*buffer_.begin() + readIndex;}

    const char* peek() const {return &*buffer_.begin() + readIndex;}

    int32_t peekInt32() const {return *((int32_t*)(&*buffer_.begin() + readIndex));}

    void printState()
    {
        LOG(DEBUG) << "Buffer::printState(): "
                   << "readIndex: " << readIndex << ", "
                   << "writeIndex: " << writeIndex << ", "
                   << "size(): " << buffer_.size() << ", "
                   << "capacity(): " << buffer_.capacity();
    }

private:

    char* begin() {
        return &*buffer_.begin();
    }

    const char* begin() const {
        return &*buffer_.begin();
    }

    std::vector<char> buffer_;
    size_t readIndex;
    size_t writeIndex;
};

}
