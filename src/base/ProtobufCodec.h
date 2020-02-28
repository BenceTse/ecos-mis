/* 
* 版权声明: PKUSZ 216 
* 文件名称 : Protebuf.h
* 创建者 :  张纪杨
* 创建日期: 2018/03/04 
* 文件描述: protobuf的编解码器
* 历史记录: 无 
*/

// protobuf的编解码器
// 编码格式用c语言struct表示：
// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   为了防止不对齐，将所有长度放到前面
//   int32_t  len;
//   int32_t  nameLen;
//   int32_t  msgLen; 
//   char     typeName[nameLen];
//   char     protobufData[msgLen];
//   char     extra[len-nameLen-msgLen-8];  // extra为附加的、廉价的、追加到尾端的数据，不会进行编码
// }

#pragma once
#include <netbase/Buffer.hpp>
#include <netbase/TcpConnection.hpp>
#include <arbit.pb.h>

class ProtobufCodec {
public:
    typedef std::shared_ptr<netbase::Buffer> BufferPtr;
    typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
    // 分发回调原型，通过parse最终将buffer解析成Message
    typedef std::function<void (const netbase::TcpConnectionPtr&,
                                MessagePtr&,
                                netbase::Timestamp,
                                BufferPtr&)>  ProtobufMessageCallback;
    ProtobufCodec(){}

    // 将msg一步步打包到buf中，buf必须要为空
    // extra为附加的、廉价的、追加到尾端的数据，不会进行编码
    static void fillEmptyBuffer(netbase::Buffer* buf, 
                                const google::protobuf::Message& msg, 
                                const char *extra = nullptr,
                                size_t len = 0);

    // 注意两种特殊情况：1. 有数据来，但是还不够组成一个完整的message，此时应该退出。
    // 2. 缓冲区内有不只一个message，必须要都读出来，否则有可能会丢失掉数据
    void onMessage(const netbase::TcpConnectionPtr& conn, netbase::Buffer *buf, netbase::Timestamp timestamp);

    void send(const netbase::TcpConnectionPtr& conn, 
              const google::protobuf::Message& message, 
              const char* extra = nullptr,
              size_t len = 0);

    void setProtobufMessageCallback(const ProtobufMessageCallback& callback) { protobufMsgCallback_ = callback; }

    // 不可拷贝
    ProtobufCodec(const ProtobufCodec&) =  delete;
    ProtobufCodec& operator=(const ProtobufCodec&) = delete;

private:
    const static int kHeaderLen = sizeof(int32_t);
    // len + nameLen + msgLen
    const static int kMinMessageLen = 3 * kHeaderLen;
    // const static int
    ProtobufMessageCallback protobufMsgCallback_; 

    // 通过message的类型名在堆上创建一个新的message
    // notes: 要创建的类型，必须要编译器已经链接过（之前已经实例化或其它）
    // PS: 1. 每个message type都有一个default instance
    // 2. 可以通过MessageFactory::GetPrototype(const Descriptor*)获得
    // 3. Descriptor*可以通过DescriptorPool::FindMessageTypeByName(const string& type_name)获得
    google::protobuf::Message* createMessage(const std::string& typeName);

    bool parse(char* ptr, size_t len, MessagePtr& msgPtr, BufferPtr& extra);
};

