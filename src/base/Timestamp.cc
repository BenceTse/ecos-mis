#include "Timestamp.hpp"
#include <sys/time.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <iostream>

namespace ecos
{
// 将int64型的时间戳转化为字符串
std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microSeconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    // 对于32位系统，64-int是 %lld
    // 对于64位系统，64-int是 %ld
    // 宏PRId64在<inttypes.h>, 对于C++必须要被定义宏"__STDC_FORMAT_MACROS"才能使用
    snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microSeconds);
    return std::string(buf);
}
// std::string Timestamp::func() const{

// }
std::string Timestamp::toFormattedFromString(const std::string &input){

    //std::string test_input = "2019-12-26T08:07:58.861463Z";

    char cur_time[128];

    struct tm tm_time;
    tm_time.tm_year = atoi(input.substr(0, 4).c_str())-1900;
    tm_time.tm_mon = atoi(input.substr(5, 2).c_str())-1;
    tm_time.tm_mday = atoi(input.substr(8, 2).c_str());
    tm_time.tm_hour = atoi(input.substr(11, 2).c_str());
    tm_time.tm_min = atoi(input.substr(14, 2).c_str());
    tm_time.tm_sec = atoi(input.substr(17, 2).c_str());
    // int microSeconds = 861463;
    mktime(&tm_time);
    // std::cout << it <<std::endl;
    strftime(cur_time, 128, "%a, %d %b %Y %H:%M:%S GMT", &tm_time);
    std::string output(cur_time);
    return output;
}


// 将int64型的时间戳转化为yyyymmdd hh-mm-ss.的形式
std::string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    // 将计数时间转换为日历时间 "struct tm"
    gmtime_r(&seconds, &tm_time);

    if(showMicroseconds)
    {
        int microSeconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d-%02d-%02dT%02d:%02d:%02d.%06dZ",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microSeconds);
    } else {
        snprintf(buf, sizeof(buf), "%4d-%02d-%02dT%02d:%02d:%02dZ",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}
// 获取当前时间的时间戳
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

}
