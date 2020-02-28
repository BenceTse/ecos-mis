
#include "AsynLogging.hpp"
#include "assert.h"
#include "CoutSink.hpp"

namespace Logger
{

class StartAsynLogging
{
public:
    StartAsynLogging()
    {
        AsynLogging::instance();
        // const char str[] = "before main\n";
        // AsynLogging::instance().append(str, sizeof(str));
    }
};
// StartAsynLogging startAsynLogging;
//开启backThread_线程，作为日志打印守护进程
AsynLogging::AsynLogging()
    : currentBuffer_(new HeapBuffer(LOG_BUF_BLOCK)),
      nextBuffer_(new HeapBuffer(LOG_BUF_BLOCK)),
      newBuffer1(new HeapBuffer(LOG_BUF_BLOCK)),
      newBuffer2(new HeapBuffer(LOG_BUF_BLOCK)),
      running_(false),
      isEnter(false),
      // backThread_("Logger Backend Thread", std::bind(&AsynLogging::backendRun, this))
      //线程名"Logger Backend Thread"， Concurrency::Thread backThread_("Logger Backend Thread", &AsynLogging::backendRun, this);
      backThread_("Logger Backend Thread", &AsynLogging::backendRun, this)
{
    // 填0
    currentBuffer_->bzero();
    nextBuffer_->bzero();

    // 后端线程使用
    newBuffer1->bzero();
    newBuffer2->bzero();
    bufferToWrite.reserve(16);

    // 添加默认的sinker
    // addDefaultSink();
    running_ = true;
    // backend线程初始化变量需要花费一段时间，如果程序刚启动就关闭，
    // 很有可能backendRun还没进入while循环running_标志已经被置为false
    backThread_.start();
}

AsynLogging::~AsynLogging()
{
    stopAndFlush(false);
}
//关闭backend线程
void AsynLogging::stopAndFlush(bool willAbort)
{
    // 如果还未进入while循环（如还在传递参数等）则等待
    {
        std::unique_lock<std::mutex> lock(isEnter_mutex_);
        while (!isEnter)
        {
            isEnter_condition_.wait(lock);
        }
        running_ = false;
    }
    // 注意两个线程同时stopAndFlush的情况
    if (backThread_.joinable())
    {
        backThread_.join();
        if (willAbort)
            abort();
    }
}

void AsynLogging::addDefaultSink()
{
    sinkVec_.emplace_back(new detail::CoutSink);
}

void AsynLogging::sinks_output(
    //sink_pointer是LogSink *类型指针的引用
    std::vector<std::unique_ptr<Buffer>> &bufferToWrite)
{
    // 遍历所有的sinker，打印输出
    for (auto it = sinkVec_.begin();
         it != sinkVec_.end();
         ++it)
    {
        std::unique_ptr<detail::LogSink> &sink_pointer = *it;
        (*sink_pointer)(bufferToWrite);
    }
}

void AsynLogging::sinks_flush()
{
    // 遍历所有的sinker，刷新到输出设备
    // condition_.notify_all();
    for (auto it = sinkVec_.begin();
         it != sinkVec_.end();
         ++it)
    {
        std::unique_ptr<detail::LogSink> &sink_pointer = *it;
        (*sink_pointer).flush();
    }
}

void AsynLogging::append(const char *logline, int len)
{
    // 双缓冲技术：A满则与B交换，继续往A写，打印B
    // 在上锁过程中只是完成交换操作，不刷新数据，缩短临界区长度，避免上锁时间过长
    {
        std::unique_lock<std::mutex> lock(mutex_);
        //如果currentBuffer剩余位置可以直接放置新增加的日志内容
        if (currentBuffer_->avail() > static_cast<size_t>(len))
        {
            currentBuffer_->append(logline, len);
        }
        else
        {
            //否则将currentBuffer_放入buffers_中
            buffers_.push_back(std::move(currentBuffer_));
            //如果nextBuffer_不为空，currentBuffer_接手nextBuffer_,如果为空则为currentBuffer_分配
            if (nextBuffer_ != nullptr)
            {
                currentBuffer_.reset(nextBuffer_.release());
            }
            else
            {
                currentBuffer_.reset(new HeapBuffer(LOG_BUF_BLOCK));
                // currentBuffer_.bzero();
            }

            currentBuffer_->append(logline, len);
            condition_.notify_one();
        }
    }
}

void AsynLogging::backendRun()
{
    {
        std::unique_lock<std::mutex> lock(isEnter_mutex_);
        isEnter = true;
        isEnter_condition_.notify_one();
        // isEnter_condition_.notify_all();
    }
    while (running_)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 如果没有日志数据，则沉睡一段时间再唤醒
            if (buffers_.empty())
            {
                //如果buffer_为空则阻塞直到接受到通知，或者flushInterval_时间之后
                condition_.wait_for(lock, std::chrono::milliseconds(flushInterval_));
            }

            // 移动语义减少拷贝操作
            //将currentBuffer放入buffer，currentBuffer_=NULL
            buffers_.push_back(std::move(currentBuffer_));
            //currentBuffer_指向newBuffer1分配的空间，newBuffer1=NULL
            currentBuffer_.reset(newBuffer1.release());

            if (nextBuffer_ == nullptr)
            {
                //nextBuffer_指向newBuffer2分配的空间，newBuffer2=NULL
                nextBuffer_.reset(newBuffer2.release());
            }
            // //交换buffers和bufferToWrite
            buffers_.swap(bufferToWrite);
        }

        sinks_output(bufferToWrite);

        if (newBuffer1 == nullptr)
        {
            assert(!bufferToWrite.empty());
            //newBuffer1指向bufferToWrite的末尾元素
            //将buffToWrite的尾部空间分配给newBuffer1
            newBuffer1.reset(bufferToWrite.back().release());
            //重新装填newBuffer1的内容
            newBuffer1->reset();
            //弹出尾部元素
            bufferToWrite.pop_back();
        }

        if (newBuffer2 == nullptr)
        {
            assert(!bufferToWrite.empty());
            newBuffer2.reset(bufferToWrite.back().release());
            newBuffer2->reset();
            bufferToWrite.pop_back();
        }

        // 清空
        bufferToWrite.clear();
        sinks_flush();
    }
}

void AsynLogging::addSink(std::unique_ptr<detail::LogSink> &&sink)
{
    if (sink != nullptr)
    {
        sinkVec_.push_back(std::move(sink));
    }
}

void AsynLogging::addSink(detail::LogSink *sink)
{
    if (sink != nullptr)
    {
        // sinkVec_.push_back(std::move(sink));
        sinkVec_.emplace_back(sink);
    }
}

} // namespace Logger
