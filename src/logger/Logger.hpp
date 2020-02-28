
#pragma once

#include "LogStream.hpp"
#include "LogSink.hpp"

// 直接打印日志
#define LOG(LEVEL)                                     \
    if (Logger::logLevel() <= Logger::LogLevel::LEVEL) \
    Logger::LoggerImpl(__FILE__, __LINE__, Logger::LogLevel::LEVEL).stream()
// 当BOOLEXP为真时打印
#define LOG_IF(LEVEL, BOOLEXP)                                    \
    if (BOOLEXP && Logger::logLevel() <= Logger::LogLevel::LEVEL) \
    Logger::LoggerImpl(__FILE__, __LINE__, Logger::LogLevel::LEVEL).stream()
// 当BOOLEXP为假时，打印日志后退出（相当于断言）
#define CHECK(BOOLEXP)                                               \
    if (!(BOOLEXP) && Logger::logLevel() <= Logger::LogLevel::FATAL) \
    Logger::LoggerImpl(__FILE__, __LINE__, Logger::LogLevel::FATAL).stream()

namespace Logger
{

// void addSink()
void addSink(detail::LogSink *sink);
void printAsynInfo();
// 等级严重程度由低到高
enum LogLevel
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS
};

// extern LogLevel logLevel();
LogLevel logLevel();
// 指定最低的打印等级
void setLogLevel(LogLevel l);
void setLogLevel(unsigned l);

class LoggerImpl
{
public:
    const char levelstr[LogLevel::NUM_LOG_LEVELS][7] = {
        "TRACE ",
        "DEBUG ",
        " INFO ",
        " WARN ",
        "ERROR ",
        "FATAL "};

    LoggerImpl(const char *file, int line, LogLevel level);

    ~LoggerImpl();

    LogStream &stream()
    {
        return stream_;
    }

private:
    // 格式化时间，将系统当前时间按照格式添加到stream_
    void fmttime();
    // 格式化tid，将线程当前ID添加到stream_
    void fmttid();
    // 格式化日志的等级
    void fmtlevel(LogLevel);
    // 根据错误码获取错误信息
    char *strerror_tl(int savedError);

    LogStream stream_;
    const char *file_;
    int line_;
    LogLevel level_;
};

} // namespace Logger
