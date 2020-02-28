/* 
* 版权声明: PKUSZ 216 
* 文件名称 : ICache.cc
* 创建者 : 张纪杨 
* 创建日期: 2018/03/04 
* 文件描述: LRU缓存，缓存文件INO和文件路径的映射
* 历史记录: 无 
*/
#include "ICache.h"
#include <assert.h>
#include <logger/Logger.hpp>

namespace arbit {
//构造函数，初始化ICache的大小
template <typename T>
ICache<T>::ICache(size_t n) : capacity(n) {
}
//缓存ino和name的对应关系
template <typename T>
void ICache<T>::cache_inode(T parent, T ino, const char* name) {
    std::string abspath; 
    if(parent == ROOT_INO) {
        abspath = "/";
    } else {
        // 如果hashMap中不存在该值，则会采用默认构造函数构造一个新的iterator
        typename std::list<CacheEnt>::iterator it = hashMap[parent];

        // 如果it与默认构造的的一样，则说明hashmap中无改元素
        assert(it != typename std::list<CacheEnt>::iterator() );
        abspath = it->second + '/';
    }
    
    abspath += name;
    typename std::list<CacheEnt>::iterator& it = hashMap[ino];
    if(it == typename std::list<CacheEnt>::iterator() ) {
        // 没有该元素的位置，新建它
        if(capacity == cacheList.size()) {
            // cache满，则删除最后一个元素
            int ret = hashMap.erase(cacheList.back().first);
            // 要不0要不1
            assert(ret == 1);
            (void)ret;
            // 删除最后一个元素
            cacheList.pop_back();
        }
        cacheList.emplace_front(ino, abspath);
        it = cacheList.begin(); 
    } else {
        // 已有该元素的位置，更新它
        it->second = abspath;
        // splace(iterator position, list&, iterator i)
        // 将i所指的元素接合于position之前。position与i可指同一个元素
        cacheList.splice(cacheList.begin(), cacheList, it);
    }
}

template <typename T>
std::string ICache<T>::get_path(T ino) {
    if(ino == ROOT_INO) return "/";
    typename std::list<CacheEnt>::iterator& it = hashMap[ino]; 
    // 如果it与默认构造的的一样，则说明hashmap中无该元素
    CHECK(it != typename std::list<CacheEnt>::iterator() ) << "item do not exist in cache";
    // 将命中的条目插到链表首部
    cacheList.splice(cacheList.begin(), cacheList, it);
    return it->second;
}

template class ICache<fuse_ino_t>;

}

