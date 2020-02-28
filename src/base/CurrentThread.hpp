/* 
* 版权声明: PKUSZ 216 
* 文件名称 : CurrentThead.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 定义当前线程所需的若干接口，Thread类实现这些接口
* 历史记录: 无 
*/
#pragma once

#include <cstdint>
#include <iostream>

namespace Concurrency
{
namespace CurrentThread
{
    //__thread变量每一个线程有一份独立实体,__thread是GCC内置的线程局部存储设施
    extern __thread pid_t cachedTid;
    // extern __thread const char* threadName;

    extern void cacheTid();
    // 获取进程id  
    extern pid_t tid();
    // {
    //     // unlikely: cachedTid == 0
    //     if(__builtin_expect( cachedTid == 0, 0))
    //     {
    //         cacheTid();
    //     }
    //     return cachedTid;
    // }
    //
    // bool isMainThread();
    void sleepUsec(int64_t usec);

}
}
