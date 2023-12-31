#pragma once

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>

#include "MqttPacket.h"



class MqttCodec : muduo::noncopyable
{
 public:
    enum ErrorCode
    {
        kNoError = 0,
        kInvalidLength,
        kCheckSumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
        kInvalidRemainingLen,
    };

    using MqttMessageCallback = std::function<void (const muduo::net::TcpConnectionPtr&,
                                                    const mqtt::MqttPacketPtr&,
                                                    muduo::Timestamp)> ;

    using ErrorCallback = std::function<void (const muduo::net::TcpConnectionPtr&,
                                              muduo::net::Buffer*,
                                              muduo::Timestamp,
                                              ErrorCode)>;

    explicit MqttCodec(const MqttMessageCallback& cb)
        : messageCallback_(cb),
          errorCallback_(defaultErrorCallback)
    {
    }

    MqttCodec(const MqttMessageCallback& messageCb, const ErrorCallback& errorCb)
        : messageCallback_(messageCb),
          errorCallback_(errorCb)
    {
    }

    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buf,
                   muduo::Timestamp receiveTime);

    void send(const muduo::net::TcpConnectionPtr& conn,
            const mqtt::MqttPacket& pkt)
    {
        
        // FIXME: serialize to TcpConnection::outputBuffer()
        muduo::net::Buffer buf;
        ErrorCode errorCode = kNoError;
        fillEmptyBuffer(&buf, pkt, &errorCode);
        if(errorCode == kNoError)
        {
            conn->send(&buf);
        }
        else
        {
            LOG_ERROR << "MqttCodec::send failure - " << errorCodeToString(errorCode);
        }

    }



    static const muduo::string& errorCodeToString(ErrorCode errorCode);
    static void fillEmptyBuffer(muduo::net::Buffer* buf, const mqtt::MqttPacket& message, ErrorCode* errorCode);
    static mqtt::MqttPacket* createMessage(const mqtt::mqtt_header& type);
    static mqtt::MqttPacketPtr parse(const char* buf, int len, int remaining_lens, ErrorCode* errorCode);

 private:

  static void defaultErrorCallback(const muduo::net::TcpConnectionPtr&,
                                   muduo::net::Buffer*,
                                   muduo::Timestamp,
                                   ErrorCode);

  MqttMessageCallback messageCallback_;
  ErrorCallback errorCallback_;

  const static int kHeaderLen = mqtt::MQTT_HEADER_LEN;
  const static int kMinMessageLen = mqtt::MQTT_HEADER_LEN; // 
  const static int kMaxMessageLen = kHeaderLen + 3 + mqtt::MAX_REMAINING_LENGHT; // 
};
