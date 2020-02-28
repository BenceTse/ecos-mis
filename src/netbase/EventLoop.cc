
/* 
* 版权声明: PKUSZ 216 
* 文件名称 : EventLoop.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: Reactor
* 历史记录: 无 
*/
#include <netbase/EventLoop.hpp>
#include <netbase/Channel.hpp>
#include <netbase/Timer.hpp>

#include <assert.h>
#include <unistd.h>
#include <signal.h>

namespace {
/* 
 * 这个类的作用是忽略管道事件 
 * 因为管道已经被使用另外的方法处理了 
 */  
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};
//用于实现对SIGPIPE信号的忽略
IgnoreSigPipe ignoreSigPipe;

}

namespace netbase
{
// 加上了__thread修饰符，表示每个线程都有一个指向Reactor的指针 
__thread EventLoop* t_loopInThisThread = 0;
// 10s
// const int kPollTimeMs = 10000;
// 1s
// 轮询的超时时间（1000毫秒 = 1秒）
const int kPollTimeMs = 1000;
/* 
 * Reactor的构造函数 
 */  
EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      threadId_(Concurrency::CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),// 调用默认的轮询器来创建轮询
      callingPendingFunctors_(false),
      wakeupfd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupfd_)),
      timerqueue_(new TimerQueue(this)),
      iteration_(0)
{
    // 打印关键信息到日志，方便调试
    LOG(TRACE) << "EventLoop created " << this << " in thread " << threadId_;
    // 确保本线程中没有其它eventloop
    if(t_loopInThisThread)
    {
        LOG(FATAL) << "Another EventLoop " << t_loopInThisThread
                   << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }

    // void (EventLoop::*pHandleRead)() = &EventLoop::handleRead;
    // (this->*pHandleRead)();
    // wakeupChannel_->setReadCallBack(this->*pHandleRead);
    // 利用eventfd唤醒本eventloop
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead, this));
    // 打开可读状态
    wakeupChannel_->enableReading();

}
/* 
 * 析构函数，销毁对象并清理 
 */
EventLoop::~EventLoop()
{
    LOG(TRACE) << "EventLoop " << this << " of thread " << threadId_
               << " destructs in thread " << Concurrency::CurrentThread::tid();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupChannel_->fd());
    t_loopInThisThread = nullptr;
}
/* 
 * 返回当前线程的Reactor对象的指针 
 */ 
EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

// 主体循环
void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    quit_ = false;
    looping_ = true;

    while(!quit_)
    {
        iteration_++;
        LOG(TRACE) << "EventLoop: iteration version - " << iteration_;
        // std::cout << "EventLoop: iteration version - "
        //     << iteration_ << std::endl;
        // 事件驱动
        // 若poll返回，则将所有有事件响应的channel塞入activeChannel
        activeChannels_.clear();
        Timestamp pollReturnTime = poller_->poll(kPollTimeMs, &activeChannels_);
        // 调用所有激活状态的channel的事件处理函数，
        // 事件处理函数进一步根据可读、可写等状态调用下属的实际的处理函数
        for(auto it = activeChannels_.begin();
            it != activeChannels_.end();
            ++it)
        {
            (*it)->handleEvent(pollReturnTime);
        }
        doPendingFunctors();
    }

    LOG(TRACE) << "EventLoop " << this << " stop looping";
    looping_ = false;

    // doPendingFunctors允许重入，可能会在进行过程中添加了新的functor，
    // 如果此时quit执行，将会直接新添加的functor不会再被执行
    // 再执行一次将剩下的工作做完
    // 递归再执行？
    int i = 0;
    while(i++ < 3 && !functors_.empty()) {
        doPendingFunctors();
    }
    // doPendingFunctors();
}


/* 
 * 让Reactor退出循环 
 * 因为调用quit的线程和执行Reactor循环的线程不一定相同，如果他们不是同一个线程， 
 * 那么必须使用wakeup函数让wakeupChannel_被激活，然后轮询函数返回，
 * 就可以处理事件，然后进入下一个循环， 
 * 由于quit_已经被设置为true，所以就会跳出循环 
 */ 
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::abortNotInLoopThread()
{
    // assert(1==0);
    LOG(FATAL) << "EventLoop " << this << " was created in threadId_ " << threadId_
               << ", current thread id = " << Concurrency::CurrentThread::tid();
}
// 更新事件处理器 
void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    // assertInLoopThread();
    poller_->updateChannel(channel);
}
// 移除一个事件处理器 
void EventLoop::removeChannel(Channel* channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);
}

// for runInLoop()
// 可跨线程调用该函数，但无论在什么线程，传入的仿函子都必然在eventloop所在的线程中执行
void EventLoop::runInLoop(const Functor& functor)
{
    // if(!looping_) return;
    if(isInLoopThread()) {
        functor();
    } else {
        // 如果本就在eventloop线程中，则直接插队运行
        queueInLoop(functor);
    }
}
// 把一个回调函数添加到投递回调函数队列中，并唤醒Reactor
void EventLoop::queueInLoop(const Functor& functor)
{
    // 压入队列后，通过wakeup方法唤醒eventloop，在loop()方法会处理该函数
    {     
        MutexLockGuard lock(mutex_);
        functors_.push_back(functor);
    }

    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::queueInLoop(Functor&& functor) 
{
    {     
        MutexLockGuard lock(mutex_);
        functors_.push_back(std::move(functor));
    }

    if(!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

// 封装::eventfd，确保处理恰当
/* 
 * 创建管道，用于支持线程之间的通信 
 */  
int EventLoop::createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG(FATAL) << "create eventfd(2) error";
    }
    return evtfd;
}
/* 
 * 唤醒 
 * 其实就是告诉正在处理循环的Reactor，发生了某一件事 
 */  
void EventLoop::wakeup()
{
    uint64_t one = 1;
    // 往eventfd写入一个uint64，eventfd会变成可读状态，
    // 从而poller返回，成功唤醒eventloop
    ssize_t n = ::write(wakeupfd_, &one, sizeof(one));
    if(n != sizeof(one))
        LOG(ERROR) << "write " << n << " bytes instead of 8";
}

// 电平触发，清空缓冲区
/* 
 * 被wakeupChannel_使用 
 * wakeupChannel_有事件发生时候（wakeup一旦被调用，wakeupChannel_就被激活），就会执行handleEvent 
 * 而handleEvent内部就会调用处理读的回调函数，wakeupChannel_的读回调函数就是handleRead 
 */  
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupfd_, &one, sizeof(one));
    if(n != sizeof(one))
        LOG(ERROR) << "read " << n << " bytes instead of 8";
}

// 处理填充的functors
// 执行投递的回调函数（投递的回调函数是在一次循环中，所有的事件都处理完毕之后才调用的） 
void EventLoop::doPendingFunctors()
{
    // 允许重入
    callingPendingFunctors_ = true;
    // 上锁，交换
    std::vector<Functor> functors;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(functors_);
    }
    // 遍历执行所有就绪的functors
    for(auto it = functors.begin();
        it != functors.end();
        ++it)
    {
        (*it)();
    }
    callingPendingFunctors_ = false;
}

// interfaces of TimerQueue
// 在指定时间点执行某项任务
TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timerqueue_->addTimer(std::move(cb), time, 0.0); 
}

// 在某段时间后执行某项任务
TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    return timerqueue_->addTimer(std::move(cb), addTime(Timestamp::now(), delay), 0.0); 
}

// 每隔一定时间重复执行某项任务
TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    return timerqueue_->addTimer(std::move(cb), addTime(Timestamp::now(), interval), interval); 
}

}
