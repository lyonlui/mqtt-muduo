#pragma once

#include "MqttPacket.h"

#include <functional>
#include <muduo/net/TcpConnection.h> 

namespace mqtt{

class MqttPacketHandle
{    
public:
    using MqttPacketCallback = std::function<void (const muduo::net::TcpConnectionPtr& conn,
                        const MqttPacketPtr& message,
                        muduo::Timestamp)>;
    using MqttPacketSend = std::function<void (const muduo::net::TcpConnectionPtr& conn,
                        const MqttPacket& message)>;
    
    explicit MqttPacketHandle(const MqttPacketCallback& defaultCb, const MqttPacketSend& send)
        : defaultCallback_(defaultCb),
          send_(send)
    {
    }

    void onMqttMessage(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt,
                       muduo::Timestamp time);
private:
    MqttPacketCallback defaultCallback_;
    MqttPacketSend send_;
    void handleConnection(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt);
    void handlePingRequest(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt);
    void handleDisconnect(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt);
    

};

}