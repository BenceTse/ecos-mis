/* 
* 版权声明: PKUSZ 216 
* 文件名称 : ProtobufCodec.cc
* 创建日期: 2018/03/04 
* 文件描述: protobuf的编解码器
* 历史记录: 无 
*/
#include "ProtobufCodec.h"
#include <string>
#include <google/protobuf/descriptor.h>
#include <netbase/Timestamp.hpp>
#include <logger/Logger.hpp>

void ProtobufCodec::fillEmptyBuffer(netbase::Buffer* buf, 
                            const google::protobuf::Message& msg, 
                            const char *extra, size_t len) 
{
    assert(buf->readableBytes() == 0);
    std::string typeName = msg.GetTypeName();
    // 名字带有终结符'\0'
    int32_t nameLen = static_cast<int32_t>(typeName.size()) + 1;
    buf->append(&nameLen, sizeof(nameLen));

    int32_t msgLen = static_cast<int32_t>(msg.ByteSize());
    buf->append(&msgLen, sizeof(msgLen));

    buf->append(typeName.c_str(), typeName.size());
    buf->append("\0", 1);
    
    uint8_t *start = static_cast<uint8_t*>(buf->writeBegin());
    uint8_t *end = msg.SerializeWithCachedSizesToArray(start);
    if(end-start != msgLen) {
        // protobuf序列化，会先计算byte size，然后才序列化
        // 如果实际写的和计算的不一致，则调用google 提供的ByteSizeConsistencyError，会crash程序
        // 一般情况下是因为计算后，改动过message的信息
        // ByteSizeConsistencyError(msgLen, msg.ByteSize(), static_cast<int>(end - start));
        LOG(FATAL) << "This shouldn't be called if all the sizes are equal.";
    }
    buf->hasWriten(msgLen);

    if(extra && len)
        buf->append(extra, len);
    int32_t totalLen = sizeof(nameLen) + sizeof(msgLen) + nameLen + msgLen + len;
    assert(totalLen == static_cast<int32_t>(buf->readableBytes()));
    buf->prepend(&totalLen, sizeof(totalLen));
    // LOG(DEBUG) << totalLen << '\t' << nameLen << '\t' << typeName << '\t' << msgLen;
}

void ProtobufCodec::send(const netbase::TcpConnectionPtr& conn, 
                         const google::protobuf::Message& message, 
                         const char* extra, size_t len)
{
    netbase::Buffer buf;
    fillEmptyBuffer(&buf, message, extra, len);
    conn->send(&buf);
}

void ProtobufCodec::onMessage(const netbase::TcpConnectionPtr& conn, 
                              netbase::Buffer *buf, 
                              netbase::Timestamp receiveTime) 
{
    LOG(TRACE) << "onMessage(): received " << buf->readableBytes() << " bytes"
               << " from connection [" << conn->name() << "] at " << receiveTime.toFormattedString();
    while(static_cast<int32_t>(buf->readableBytes()) >= kMinMessageLen) {
        int32_t len = buf->peekInt32();
        if(len + sizeof(len) <= buf->readableBytes()) {
            // 至少有一个数据项就位
            MessagePtr msg; BufferPtr extra;
            if(parse(buf->peek()+sizeof(len), len, msg, extra)) {
                if(protobufMsgCallback_){
                    protobufMsgCallback_(conn, msg, receiveTime, extra);
                }
                buf->retrieve(kHeaderLen + len);
            } else {
                LOG(FATAL) << "something wrong must happen";
            }
        } else 
            break;
    }
}

google::protobuf::Message* ProtobufCodec::createMessage(const std::string& typeName)
{
    google::protobuf::Message* message = NULL;
    const google::protobuf::Descriptor* descriptor = 
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
    if (descriptor)
    {
        const google::protobuf::Message* prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype)
        {
            message = prototype->New();
        }
    }
    return message;
}


bool ProtobufCodec::parse(char* ptr, size_t len, MessagePtr& msgPtr, BufferPtr& extra) 
{
    assert(!msgPtr && !extra);
    int32_t nameLen = *((int32_t*)ptr);
    ptr += sizeof(nameLen); len -= sizeof(nameLen);

    int32_t msgLen = *((int32_t*)ptr);
    ptr += sizeof(msgLen); len -= sizeof(msgLen);

    std::string typeName(ptr, ptr + nameLen - 1);
    ptr += nameLen; len -= nameLen;

    msgPtr.reset(createMessage(typeName));
    if(msgPtr) {
        if(msgPtr->ParseFromArray(ptr, msgLen)) {
            ptr += msgLen; 
            if(static_cast<int32_t>(len) < msgLen) {
                LOG(ERROR) << "The size of extra should not be negative.";
                return false;
            }
            len -= msgLen;
            if(len > 0) {
                extra.reset(new netbase::Buffer());
                extra->append(ptr, len);
            }
            return true;
        } else {
            LOG(ERROR) << "protobuf parse error";
        }
    } else {
        // 如果发现这个错误，请确保是否调用了Dispatcher::registerMessageCallback<SomeMessageType>函数
        // 也就是说，必须要曾经使用过要解析message类，编译器才会去链接它
        // 如果不链接它，descriptor池中是不会有这个类型的全局指针的
        LOG(ERROR) << "unknown message type";
    }
    return false;
    // LOG(INFO) <<
}

