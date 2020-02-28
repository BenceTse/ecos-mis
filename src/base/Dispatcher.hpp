/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Dispatcher.hpp
* 创建日期: 2018/03/04 
* 文件描述: 分发器：将解析后的google::protobuf::message，根据类型分发到不同的回调函数中
* 历史记录: 无 
*/
// 分发器：将解析后的google::protobuf::message，根据类型分发到不同的回调函数中
// 为了实现：客户代码拿到的就已经是想要的具体类型
// 1. 通过message的descriptor()方法（该方法返回对于该类型的唯一指针），可以获得key
// 2. 最直接的方式是采用std::map<descriptor*, std::function<Args...> >，
//    然而注意到std::function<void(google::protobuf::Message)>是无法down-cast成std::function<void(concreteMessage)>
// 3. 更好的方式是通过运行时多态与模板结合，采用std::map<descriptor*, __Callback_>的形式

#pragma once
#include <map>
#include <functional>
#include <memory>

#include <netbase/Buffer.hpp>
#include <google/protobuf/message.h>
#include <netbase/TcpConnection.hpp>
#include <netbase/Timestamp.hpp>
#include "ThreadPool.h"

namespace arbit {

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<netbase::Buffer> BufferPtr;

// 4. __Callback_作为抽象父类, 所有__CallbackT_被up-cast为父类放入std::map
// 5. 在运行时可动态down-cast为原有的子类
class __Callback_ {
public:
    // 要不使用虚析构，要不就用智能指针管理
    // 不能是纯虚的，因为子类调用析构会默认调用父类的析构
    virtual ~__Callback_() {};
    virtual void onMessage(const netbase::TcpConnectionPtr&,
                      MessagePtr&,
                      const netbase::Timestamp&,
                      BufferPtr&) = 0;
};

// 思路：利用模板类推导出具体的message类型T，由T生成std::function(实际就是回调函数)的类型
//       同样T也为down-cast后类型
template<typename T>
class __CallbackT_ : public __Callback_ {
public:
    typedef std::function<void(const netbase::TcpConnectionPtr&,
                               std::shared_ptr<T>,
                               const netbase::Timestamp&,
                               BufferPtr&)> ProtobufMessageCallback;
    __CallbackT_(const ProtobufMessageCallback callback, ThreadPool* threadPool) : 
        callback_(callback),
        threadPool_(threadPool)
    {}

    ~__CallbackT_() override {}

    void onMessage(const netbase::TcpConnectionPtr& conn,
                   MessagePtr& message,
                   const netbase::Timestamp& receiveTime,
                   BufferPtr& extra) override
    {
        LOG(TRACE) << "the received type: " << message->GetTypeName() << ", " << message->ByteSize() << " bytes, "
                   << "extra data size: " << (extra?extra->readableBytes():0) << " bytes, "
                   << "from connection [" << conn->name() << "] at " << receiveTime.toFormattedString();
        // 此处如果确保没有问题，可以在实际发行版本中采用static_pointer_cast，这样就没有RTTI开销
        std::shared_ptr<T> msg = std::dynamic_pointer_cast<T>(message);
        // 此处绝对不能用dynamic_cast，否则会出现两次析构
        // std::shared_ptr<T> msg(dynamic_cast<T*>(&*message));
        CHECK(msg != nullptr) << "down cast error";
        if(threadPool_)
            threadPool_->enqueue(callback_, conn, msg, receiveTime, extra);
        else if(callback_) {
            callback_(conn, msg, receiveTime, extra);
        }
    }
private:
    ProtobufMessageCallback callback_;
    ThreadPool* threadPool_;
};

class Dispatcher {
public:
    Dispatcher(ThreadPool* threadPool) : threadPool_(threadPool) {}

    ~Dispatcher() 
    {
        for(auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
            assert(it->second != nullptr);
            delete it->second;
        }
    }

    template<typename T>
    void registerMessageCallback(const typename __CallbackT_<T>::ProtobufMessageCallback& messageCallback_) {
        __CallbackT_<T>* p = new __CallbackT_<T>(messageCallback_, threadPool_);
        callbacks_.emplace(T::descriptor(), p);
    }

    void onMessage(const netbase::TcpConnectionPtr& conn,
                   MessagePtr& message,
                   const netbase::Timestamp& receiveTime,
                   BufferPtr& extra) 
    {
        auto it = callbacks_.find(message->GetDescriptor());
        if(it != callbacks_.end()) {
            // __CallbackT_里的onMessage会判断是否设置了线程池变量
            it->second->onMessage(conn, message, receiveTime, extra);
            // if(threadPool_) {
                // threadPool_->enqueue(it->second->onMessage, it->second, conn, message, receiveTime, extra);
                // it->second->onMessage(conn, message, receiveTime, extra);
            // }
        } else {
            LOG(ERROR) << "has not registered messsageCallback";
        }
    }
private:
    // FIXME: 普通指针析构？
    typedef std::map<const google::protobuf::Descriptor*, __Callback_*> CallbackMap;

    CallbackMap callbacks_;
    ThreadPool* threadPool_ = nullptr;
};

}
