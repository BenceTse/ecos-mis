/* 
* 版权声明: PKUSZ 216 
* 文件名称 : EventLoopThreadPool.cc
* 创建日期: 2018/03/04 
* 文件描述: 事件循环线程池,这个类适用于一个进程中存在多个Reactor实例的情况 
* 历史记录: 无 
*/
#include <netbase/EventLoop.hpp>
#include <netbase/EventLoopThread.hpp>
#include <netbase/EventLoopThreadPool.hpp>
#include <assert.h>

namespace netbase
{
/* 
 * 构造函数 
 */ 
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
    : baseLoop_(baseLoop),
      name_(name),
      next_(0),
      nloops_(0),
      started_(false)
{
}
// 启动线程池
void EventLoopThreadPool::start()
{
    // 创建指定数量的线程，并启动
    for(size_t i = 0; i < nloops_; i++)
    {
        EventLoopThread* loopThread = new EventLoopThread(name_ + "#" + std::to_string(i),
                                                          threadInitCallback_);
        loopThreads_.emplace_back(loopThread);
        loops_.push_back(loopThread->startLoop());
    }
    started_ = true;
}
// 获取下一个EventLoop对象
EventLoop* EventLoopThreadPool::getNextLoop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    assert(started_);
    if(nloops_ == 0)
        return nullptr;
    else if(next_ == static_cast<int>(nloops_))
        next_ = 0;
    return loops_[next_++];
}

}
