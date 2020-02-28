/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Condition.h
* 创建日期: 2018/03/04 
* 文件描述: 条件变量的封装
* 历史记录: 无 
*/
#pragma once
#include "MutexLockGuard.h"
class Condition {
public:
    explicit Condition(MutexLock& mutex) : mutex_(mutex)
    {
        // 初始化 
        pthread_condattr_init(&condattr_);
        if(mutex.shared_){
            pthread_condattr_setpshared(&condattr_, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&cond_, &condattr_);
        }else {
            pthread_cond_init(&cond_, NULL);
        }
    }
    ~Condition(){
        // 释放 
       pthread_cond_destroy(&cond_);
       pthread_condattr_destroy(&condattr_);
    }
    // 唤醒一个等待的线程  
    /* 
     * pthread_cond_signal在多处理器上可能同时唤醒多个线程， 
     * 当你只能让一个线程处理某个任务时，其它被唤醒的线程就需要继续 wait, 
     * while循环的意义就体现在这里了， 
     * 而且规范要求pthread_cond_signal至少唤醒一个pthread_cond_wait上 的线程， 
     * 其实有些实现为了简单在单处理器上也会唤醒多个线程 
     */
    void notify() {
        pthread_cond_signal(&cond_);
    }
    /* 
     * 唤醒所有等待的线程 
     */ 
    void notifyAll() {
        pthread_cond_broadcast(&cond_);
    }

    template<typename F>
    void wait(F&& f){
        while(!f())
            pthread_cond_wait(&cond_, mutex_.getPthreadMutex());
    }
private:
    MutexLock& mutex_;// 条件变量的锁  
    pthread_condattr_t condattr_;
    pthread_cond_t cond_;// 条件变量 
};
