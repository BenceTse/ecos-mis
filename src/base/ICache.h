/* 
* 版权声明: PKUSZ 216 
* 文件名称 : ICache.h
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: LRU缓存，缓存文件INO和文件路径的映射
* 历史记录: 无 
*/
/*

1)	Why？
    a)	Cephfs用fuse_ino_t唯一标识一个文件，而不同fs同一个路径下的同一个文件的fuse_ino_t不一定是一样的。
    b)	不同fs之间唯一相同的是路径名称
    c)	对于write、read等函数，它们只认识fuse_ino_t
    d)	由文件句柄（fuse_ino_t）逆推文件路径对于任何fs来说都是艰难的

2)	How？
    a)	既然很难从fuse_ino_t逆推path，则最好的方法是在调用lookup等函数时，就把fuse_ino_t和path的map关系cache下来。
    b)	到实际write/read等函数时，由于它们只认识fuse_ino_t，则可以在cache中通过fuse_ino_t查找对应的fuse_ino_t
    c)	考虑到该cache的局部性非常明显，cache的命中率有保障，且cache的大小不会很大。
    d)	LRU(Least recently used) cache非常符合我们的需求

3)	数据结构设计
    a)	常见lru的数据结构为链表。
    b)	仅用链表认为查询效率（线性时间）较差。
    c)	常用的方法为配合散列表。散列表的查询在最坏情况下会退化为线性时间。然而绝大部分情况，复杂度为常量时间

*/

#pragma once
#include <string>
#include <list>
#include <unordered_map>
// #include <fuse_lowlevel.h>

#define ROOT_INO    1

// extern template class ICache<fuse_ino_t>;

// FIXME: 改为模板类
namespace arbit {

template <typename T>
class ICache {
public:
    // using namespace std;
    ICache(size_t n);

    // 不可拷贝
    ICache(const ICache&) = delete;
    ICache& operator=(const ICache&) = delete;

    // 根据fuse_ino_t获取对应的路径名
    std::string get_path(T);
    // 将fuse_ino_t与map的映射关系缓存下来
    void cache_inode(T, T, const char* name);

private:
    typedef std::pair<T, std::string> CacheEnt;

    std::list<CacheEnt> cacheList;
    std::unordered_map<T, typename std::list<CacheEnt>::iterator> hashMap;

    size_t capacity;
};

// extern template class ICache<fuse_ino_t>;

}
