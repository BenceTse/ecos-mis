
#pragma once
#include <stdint.h>
#include <string>

namespace ecos 
{

class Timestamp
{
public:

    static const int64_t kMicroSecondsPerSecond = 1000 * 1000;
    //默认构造函数，时间戳初始化为0
    Timestamp()
        : microSecondsSinceEpoch_(0)
    {
    }

    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {
    }

    void swap(Timestamp& that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // defalut copy/assignment/dtor(解构) are Okey
    // 将int64型的时间戳转化为字符串
    std::string toString() const;
    static std::string toFormattedFromString(const std::string &input);
    // 将int64型的时间戳转化为yyyymmdd hh-mm-ss.的形式
    std::string toFormattedString(bool showMicroseconds = true) const;
    // 获取时间戳，微妙精度
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    // 获取时间戳，秒的精度
    time_t secondsSinceSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }
    // 获取当前时间
    static Timestamp now();
    // 重置时间戳
    static Timestamp invalid()
    {
        return Timestamp();
    }
    // 判断时间戳是否合法
    bool valid()
    {
        return microSecondsSinceEpoch_ != 0;
    }
    // 将time_t型的时间戳转化为int64_t
    static Timestamp fromUnixTime(time_t t, int microSeconds = 0)
    {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microSeconds);
    }

private:
    int64_t microSecondsSinceEpoch_;
};
//  '<' 运算符重载
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}
//  '==' 运算符重载
inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}
//  '!=' 运算符重载
inline bool operator!=(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() != rhs.microSecondsSinceEpoch();
}
// 计算两个Timestamp相差的秒数
inline double timeDifference(Timestamp big, Timestamp small)
{
    int64_t diff = big.secondsSinceSinceEpoch() - small.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}
// 计算Timestamp加上seconds后的结果
inline Timestamp addTime(const Timestamp& timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds) * Timestamp::kMicroSecondsPerSecond;
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}
