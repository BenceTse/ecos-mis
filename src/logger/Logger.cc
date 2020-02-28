#include <string.h>
#include <time.h>
#include "Logger.hpp"
#include "AsynLogging.hpp"
#include "base/CurrentThread.hpp"

namespace Logger
{

// static LogLevel g_level = LogLevel::DEBUG;
// 全局过滤等级默认为最低
static LogLevel g_level = LogLevel::TRACE;

// 线程变量，保存指定错误码的字符描述信息
__thread char t_errnobuf[512];

void addSink(detail::LogSink *sink)
{
    AsynLogging::instance().addSink(sink);
}
void printAsynInfo()
{
    char buf[1000] = {0};
    AsynLogging &al = AsynLogging::instance();
    snprintf(buf, 1000, "-----------sink.size(): %ld, \n", al.sinkVec_.size());
    AsynLogging::instance().append(buf, strlen(buf));
}

LogLevel logLevel()
{
    return g_level;
}

// 指定最低的打印等级
void setLogLevel(LogLevel l)
{
    g_level = l;
}

void setLogLevel(unsigned l)
{
    if (l >= LogLevel::NUM_LOG_LEVELS)
        return;
    setLogLevel(static_cast<LogLevel>(l));
}

// 格式化时间
void LoggerImpl::fmttime()
{
    time_t t = time(0);
    struct tm *now = localtime(&t);

    char buf[32];
    snprintf(buf, 32, "%4d%02d%02d %02d:%02d:%02d ",
             // tm_year从1900开始计算偏移
             now->tm_year + 1900,
             now->tm_mon + 1,
             now->tm_mday,
             now->tm_hour,
             now->tm_min,
             now->tm_sec);

    stream_ << buf;
}

// 格式化tid
void LoggerImpl::fmttid()
{
    char buf[16];
    snprintf(buf, 15, "%5d ", Concurrency::CurrentThread::tid());
    stream_ << buf;
}

// 格式化日志的等级
void LoggerImpl::fmtlevel(LogLevel level)
{
    stream_ << levelstr[level];
}

// 根据错误码获取错误信息
char *LoggerImpl::strerror_tl(int savedError)
{
    // strerror_r函数根据errno值给出相应的描述信息，描述信息要不就存在用户给的缓存区中，
    // 要不就存在某个静态string中（意味着用户缓存区没有被使用）
    return strerror_r(savedError, t_errnobuf, sizeof(t_errnobuf));
}

LoggerImpl::LoggerImpl(const char *file, int line, LogLevel level)
    : file_(file),
      line_(line),
      level_(level)
{
    // errno是线程变量，故是线程安全的
    int savedError = errno;
    fmttime();
    fmttid();
    fmtlevel(level);
    stream_ << '[';
    if (level >= LogLevel::ERROR && savedError != 0)
    {
        stream_ << strerror_tl(savedError) << " (errno=" << savedError << ") ";
    }
}

LoggerImpl::~LoggerImpl()
{
    // 自动记录打印点的位置
    const char *p = strrchr(file_, '/');
    if (p)
    {
        // stream_ << " - " << p + 1 << ':';
        stream_ << "] " << p + 1 << ':';
        stream_ << line_;
    }
    // 自动换行
    stream_ << '\n';
    auto &entry = stream_.getEntry();
    // 追加到输出缓冲区
    Logger::AsynLogging::instance().append(entry.c_buf(), entry.size());
    // 如果错误时致命的，则直接退出程序
    if (level_ >= LogLevel::FATAL)
    {
        Logger::AsynLogging::instance().stopAndFlush(true);
        // abort();
    }
}

} // namespace Logger
