#pragma once

// 在头文件中声明，在实现中定义
#ifndef GLOBAL_DEFINE
#define EXTERN extern
#else
#define EXTERN
#endif

// 测试用的全局计数器，以及对应的锁
// EXTERN std::mutex g_ct_mutex;

namespace ecos_mis
{

// EXTERN netbase::EventLoop *g_message_queue_loop;

} // namespace ecos_mis
