/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Thread.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 线程类，对pthread的封装
* 历史记录: 无 
*/
#pragma once

#include "CurrentThread.hpp"
#include <functional>
#include <string>
#include <thread>
#include <mutex>

namespace Concurrency
{

class Thread 
{
public:
    // 线程回调函数
    typedef std::function<void ()> ThreadFunc; 
    
    template<typename F, typename... Args>
    Thread(const std::string& n, F&& f, Args&&... args);

    // Thread(const std::string& n = "unknown name");
    // template<typename F, typename... Args>
    // Thread(F&& f, Args&&... args);

    ~Thread();

    // noncopyable
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    // 启动线程 
    void start();
    // 判断线程是否已经启动
    bool started();
    // 等待线程结束
    void join();
    // 是否可以join
    bool joinable();
    // 获取进程id
    pid_t tid();
    
private:
    void threadRun();

    bool started_;// 是否已经开始执行  
    bool joined_;// 等待结束  
    std::string name_;// 线程id  
    std::thread *innerThread_;
    // pthread_t pthreadId_;
    ThreadFunc func_;// 线程函数（执行线程任务的地方） 
    pid_t tid_;// 线程id  

    std::mutex mutex_;
};
//模板类，线程的构造函数
template<typename F, typename... Args>
Thread::Thread(const std::string& n, F&& f, Args&&... args)
    : started_(false),
      joined_(false),
      name_( (n == "") ? "NullName":n),
      innerThread_(nullptr),
      func_(std::bind(std::forward<F>(f), std::forward<Args>(args)...)),
      tid_(0)
{
    // func_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
}

// template<typename F, typename... Args>
// Thread::Thread(F&& f, Args&&... args)
//     : Thread("unname thread", std::forward<F>(f), std::forward<Args>(args)...)
// {
//
// }

}
