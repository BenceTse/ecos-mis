/* 
* 版权声明: PKUSZ 216 
* 文件名称 : ThreadPool.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 线程池类
* 历史记录: 无 
*/
#include "ThreadPool.h"


// num: 指定最大的工作线程数
ThreadPool::ThreadPool(size_t num)
    : stop(false)
{
    for(size_t i = 0; i < num; i++)
    {
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        // 上锁、阻塞
                        this->condition.wait(lock,
                                [this]{ return this->stop || !this->tasks.empty(); } );
                        if( this->stop && this->tasks.empty() )
                            return;

                        // 若有job入队，则取出来执行
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();

                }
            }
        );
    }
}

//线程池析构函数
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    // 析构时必须要将所有worker完成
    for(std::thread &worker : workers)
        worker.join();
}
