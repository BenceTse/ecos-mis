/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Atomic.h
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 原子计数器
* 历史记录: 无 
*/
#pragma once

#include <stdint.h>

/*
   *    所有操作都是原子操作，不必担心临界问题
   *    利用gcc/g++提供的基础设施库
   */

namespace netbase
{

template<typename T>
class AtomicIntegerT
{
public:

    AtomicIntegerT()
        : value_(0)
    {
    }

    // 不可拷贝
    AtomicIntegerT(const AtomicIntegerT<T>&) = delete;
    AtomicIntegerT<T>& operator=(const AtomicIntegerT<T>&) = delete;

    // uncomment if you need copying and assignment
    //
    // AtomicIntegerT(const AtomicIntegerT& that)
    //   : value_(that.get())
    // {}
    //
    // AtomicIntegerT& operator=(const AtomicIntegerT& that)
    // {
    //   getAndSet(that.get());
    //   return *this;
    // }

    // 获取计数器值
    T get()
    {
        // 如果gcc的版本大于4.7（包括），采用__atomic_load_n(&value_, __ATOMIC_SEQ_CST)
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    //先获取，之后再加，类似于i++
    T getAndAdd(T x)
    {
        // 如果gcc的版本大于4.7（包括），采用__atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
        return __sync_fetch_and_add(&value_, x);
    }

    // 先加再获取，类似于++i
    T addAndGet(T x)
    {
        return getAndAdd(x) + x;
    }

    // 递增后获取
    T incrementAndGet()
    {
        return addAndGet(1);
    }

    // 递减后获取
    T decrementAndGet()
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }

    void decrement()
    {
        decrementAndGet();
    }

    // 获取当前值后设置一个新值
    T getAndSet(T newValue)
    {
        // in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
        return __sync_lock_test_and_set(&value_, newValue);
    }

private:
    volatile T value_;
};

    // 32位整型
    typedef AtomicIntegerT<int32_t> AtomicInt32;
    // 64位整型
    typedef AtomicIntegerT<int64_t> AtomicInt64;
}
