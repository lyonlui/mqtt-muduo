#include "MqttPacketHandle.h"

using namespace mqtt;

void MqttPacketHandle::onMqttMessage(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt,
                       muduo::Timestamp time)
{

    LOG_INFO << "MqttPacketHandle:onMqttMessage " << pkt->toString();

    switch (pkt->getType())
    {
    case mqtt::CONNECT :
        handleConnection(conn, pkt);
        break;
    case mqtt::SUBSCRIBE :
        break;
    case mqtt::UNSUBSCRIBE :
        break;
    case mqtt::PUBLISH :
        break;
    case mqtt::PUBACK :
        break;
    case mqtt::PUBREC :
        break;
    case mqtt::PUBREL :
        break;
    case mqtt::PUBCOMP:
        break;
    case mqtt::PINGREQ :
        handlePingRequest(conn, pkt);  
        break;
    case mqtt::DISCONNECT :

        break;
    default:        //unknow packet
        defaultCallback_(conn, pkt, time);
        break;
    }
    
}

void MqttPacketHandle::handleConnection(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt)
{
    //MqttPingResp resp(makeHeader(packet_type::PINGRESP));
    ConnAckFlag flag;
    flag.bits.session_present = false;

    MqttConnack ack(makeHeader(packet_type::CONNACK), ConnReturnCode::ConnAccept, flag);
    send_(conn, ack);
    

    //TODO 设置保持连接定时器
}

void MqttPacketHandle::handlePingRequest(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt)
{
    MqttPingResp resp(makeHeader(packet_type::PINGRESP));
    send_(conn, resp);

    //TODO 更新保持连接定时器
}

void handleDisconnect(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt)
{

    //TODO 清理会话 session

    //TODO 必须丢弃任何与当前连接关联的未发布的遗嘱消息，具体描述见 3.1.2.5节 [MQTT-3.14.4-3]。


    // 清理session之后，客户端还没关闭连接的话，由服务器端关闭
    if(conn->connected())
        conn->shutdown();
}                       