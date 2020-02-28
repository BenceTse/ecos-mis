/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Buffer.cc
* 创建日期: 2018/03/04 
* 文件描述: 动态增长的数组
* 历史记录: 无 
*/
#include <netbase/Buffer.hpp>
#include <sys/uio.h>

namespace netbase
{
//从文件描述符fd中读数据
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char buf[65536];
    struct iovec vec[2];

    size_t writable = writableBytes();
    vec[0].iov_base = &*buffer_.begin() + writeIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = buf;
    vec[1].iov_len = sizeof(buf);

    ssize_t n = readv(fd, vec, 2);
    if(n < 0) { 
        *savedErrno = errno;
    } else if(n <= static_cast<ssize_t>(writable)) {
        writeIndex += n;
    } else {
        writeIndex = buffer_.size();
        append(buf, n - writable);
    }

    return n;
}

}
