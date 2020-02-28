/* 
* 版权声明: PKUSZ 216 
* 文件名称 : utils.cc
* 创建日期: 2018/03/04 
* 文件描述: 字符串工具类
* 历史记录: 无 
*/
#include "utils.hpp"
#include <unistd.h>
#include <assert.h>

#include <iostream>

namespace utils
{
long stol(const std::string &str){
    long result;
    std::istringstream iss(str);
    iss >> result;
    return result;
}

std::string& trim(std::string &s) 
{
    if (s.empty()) 
    {
        return s;
    }

    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}


void fork_exec(const char* cmd) {
    int pid = fork();
    if(pid == 0) {
        // 子进程
        int ret = execl("sh", "sh", "-c", cmd);
        assert(ret == 0);
        exit(ret);
    } 
}

void fork_exec(const std::string& cmd) {
    fork_exec(cmd.c_str());
    /*
    int ret = 0;
    const char* cmd = _cmd.c_str();
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    auto exitstat = pclose(pipe);
    ret = WIFEXITED(exitstat) ? WEXITSTATUS(exitstat) : -1;
    return ret;
    */
}

int Popen(const std::string& _cmd) {
    int ret = 0;
    const char* cmd = _cmd.c_str();
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if(pipe == nullptr) return 1;
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    auto exitstat = pclose(pipe);
    // WIFEXITED(status) 若此值为非0 表明进程正常结束
    // 若WIFEXITED(status)为非0，则通过WEXITSTATUS(status)来获取进程退出状态
    // PS：不知道为什么有时候下面三行会影响到exitstat的值
    // std::cout << "!#@!$#@$#$#\t" << exitstat << std::endl;
    // std::cout << "!#@!$#@$#$#\t" << WIFEXITED(exitstat) << std::endl;
    // std::cout << "!#@!$#@$#$#\t" << WEXITSTATUS(exitstat) << std::endl;
    if(WIFEXITED(exitstat) && WEXITSTATUS(exitstat) == 0) return 0;
    else return 1;
    // ret = WIFEXITED(exitstat) ? WEXITSTATUS(exitstat) : -1;
    return ret;
}

// const char digits[] = "9876543210123456789";
// const char * const zero = digits + 9;

const char digitsHex[] = "0123456789abcdef";
// const char digits[] = "9876543210123456789";
// const char * const zero = digits + 9;

size_t convertHex(char buf[], uintptr_t value)
{
    char* p = buf;
    do 
    {
        int lsd = static_cast<int>(value & 0xF);
        *p++ = digitsHex[lsd];
        value >>= 4 ;
    } while(value);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}
//将字符串str按照delim分割为字符数组
void split(std::vector<std::string> &res, const std::string &str, char delim){
    std::stringstream sstr(str);
    std::string item;
    while(std::getline(sstr, item, delim)){
        if(!item.empty())
            res.push_back(item);
    }
}

std::vector<std::string> split(const std::string &str, char delim){
    std::vector<std::string> res;
    split(res, str, delim);
    return res;
}

/**
 * 将string转成int
 * @param str
 * @return
 */
int string_to_int(const std::string &str){
    std::stringstream ss;
    ss << str;
    int res;
    ss >> res;
    return res;
}

/**
 * 检查src字符串中是否包含contain_str字符串
 * @param src
 * @param contain_str
 * @return
 */
bool isContainStr(std::string src, std::string contain_str){
    std::string::size_type idx = src.find(contain_str);
    if(idx != std::string::npos){
        return true;
    }else{
        return false;
    }
}




}
