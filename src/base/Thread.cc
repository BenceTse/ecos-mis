/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Thread.cc
* 创建日期: 2018/03/04 
* 文件描述: 线程类，对pthread的封装
* 历史记录: 无 
* notes: 仅仅是测试使用，发行版请使用std::thread
*/
#include <unistd.h>
#include <sys/syscall.h>

#include "Thread.hpp"
#include <cassert>
#include <logger/Logger.hpp>

namespace Concurrency
{

namespace CurrentThread
{
    // tid不可能为0，第一个进程的id为1
    __thread pid_t cachedTid_ = 0;
    __thread const char* threadStatus_ = "unkown";
    // 缓存线程ID
    void cacheTid()
    {
        if( cachedTid_ == 0)
        {
            // get thread_id by syscall
            cachedTid_ = static_cast<pid_t>( ::syscall(SYS_gettid) );    // explict global namespace
        }
    }

    pid_t tid()
    {
        // unlikely: cacheTid == 0
        if(__builtin_expect(cachedTid_ == 0, 0))
        {
            cacheTid();
        }
        return cachedTid_;
    }

}
// 执行线程函数  
void Thread::threadRun()
{
    // tid_ 第一次获取通过系统调用
    // 存在的竟态条件：
    //     tid_获取内陷到内核时，外部调用Thread::tid()时，tid_值并不确定
    tid_ = CurrentThread::tid(); 

    try
    {
        func_();
        CurrentThread::threadStatus_ = "finished";
    }
    catch(const std::exception& ex)
    {
        CurrentThread::threadStatus_ = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        // ?
        abort();
    }
    catch(...)
    {
        CurrentThread::threadStatus_ = "crashed";
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw;
    }
}
/* 
 * 启动线程 
 */  
void Thread::start()
{
    // 断言线程还没有执行
    assert(!started_);
    started_ = true;
    // innerThread_ = new std::thread(std::bind(&threadRun, this));
    // 创建线程，调用了threadRun
    innerThread_ = new std::thread(std::bind(&Thread::threadRun, this));
}
// 判断线程是否已经启动
bool Thread::started()
{
    return started_;
}
/* 
 * 等待线程结束 
 */ 
void Thread::join()
{
    assert(started_);
    assert(joinable());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        joined_ = true;
    }

    assert(innerThread_ != nullptr);
    innerThread_->join();
}

bool Thread::joinable() {
    std::lock_guard<std::mutex> lock(mutex_);
    return joined_ == false;
}

// 获取线程id  
pid_t Thread::tid()
{
    return tid_;
}

Thread::~Thread()
{
    if(started_ && joinable())
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            joined_ = true;
        }
        innerThread_->join();
    }
    if(innerThread_ != nullptr)
    {
        delete innerThread_;
    }
}

// template<typename F, typename... Args>
// Thread::Thread(const std::string& n, F&& f, Args&&... args)
// {
//
// }

}
