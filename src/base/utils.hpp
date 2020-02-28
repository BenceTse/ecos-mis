/* 
* 版权声明: PKUSZ 216 
* 文件名称 : utils.hpp
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: 字符串工具类
* 历史记录: 无 
*/
#pragma once

#include <cstdint>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

namespace utils
{
long stol(const std::string &str);

std::string& trim(std::string &s);

void fork_exec(const char* cmd);

void fork_exec(const std::string& cmd);

int Popen(const std::string& _cmd);

// extern size_t convertHex(char buf[], uintptr_t value);
size_t convertHex(char buf[], uintptr_t value);

void split(std::vector<std::string> &res, const std::string &str, char delim);

std::vector<std::string> split(const std::string &str, char delim);

const char digits[] = "9876543210123456789";
const char * const zero = digits + 9;
// extern const char digits[];
// extern char * const zero;
//将T类型转换为支付串类型
template<typename T>
size_t convert(char buf[], T value)
{
    T tmp = value;
    char* p = buf;

    do 
    {
        int lsd = static_cast<int>(tmp % 10);
        *p++ = zero[lsd];
        tmp /= 10;
    } while(tmp);

    if(value < 0)
    {
        *p++ = '-';
    }

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

}
