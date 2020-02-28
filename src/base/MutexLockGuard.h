/* 
* 版权声明: PKUSZ 216 
* 文件名称 : MutexLockGuard.h
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 互斥锁的封装
* 历史记录: 无 
*/
#pragma once
#include <pthread.h>

class MutexLock {
public:
    friend class MutexLockGuard;
    friend class Condition;
    MutexLock(bool shared) : shared_(shared) {
        // 初始化互斥锁，并断言操作是否成功  
        pthread_mutexattr_init(&mutexattr_);
        if(shared_){
            pthread_mutexattr_setpshared(&mutexattr_, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&mutex_, &mutexattr_);
        }else {
            pthread_mutex_init(&mutex_, NULL);
        }
    }
    MutexLock() : MutexLock(false) { }

    ~MutexLock() {
        pthread_mutexattr_destroy(&mutexattr_);
        pthread_mutex_destroy(&mutex_);
    }
private:
    // 加锁 
    void lock(){
        pthread_mutex_lock(&mutex_);
    }
    // 解锁
    void unlock() {
        pthread_mutex_unlock(&mutex_);
    }
    pthread_mutex_t* getPthreadMutex(){
        return &mutex_;
    }
    pthread_mutex_t mutex_;
    pthread_mutexattr_t mutexattr_;
    bool shared_;
};
//这个类的作用是安全的进行加锁解锁
class MutexLockGuard {
public:
    explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex) {
        mutex_.lock();
    }
    ~MutexLockGuard(){
        mutex_.unlock();
    }
private:
    MutexLock& mutex_;
};
