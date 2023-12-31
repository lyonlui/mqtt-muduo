#include "MqttServer.h"


#include <set>
#include <stdio.h>
#include <unistd.h>



using namespace muduo;
using namespace muduo::net;

  
using namespace mqtt;


MqttServer::MqttServer(EventLoop* loop,
             const InetAddress& listenAddr)
  : server_(loop, listenAddr, "MqttServer"),
    packetHandle_(std::bind(&MqttServer::onUnknownMessage, this, _1, _2, _3), std::bind(&MqttServer::send, this, _1, _2)),
    codec_(std::bind(&MqttPacketHandle::onMqttMessage, &packetHandle_, _1, _2, _3))
  {
    server_.setConnectionCallback(
        std::bind(&MqttServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&MqttCodec::onMessage, &codec_, _1, _2, _3));
  }

void MqttServer::start()
{
  server_.start();
}


void MqttServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG_INFO << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  
}

void MqttServer::onUnknownMessage(const TcpConnectionPtr& conn,
                      const MqttPacketPtr& message,
                      Timestamp)
{
  LOG_INFO << "onUnknownMessage: " << message->getType();
  conn->shutdown();
}

void MqttServer::send(const muduo::net::TcpConnectionPtr& conn,
            const mqtt::MqttPacket& pkt)
{
  LOG_INFO << "MqttServer:send() send package " << pkt.toString();
  codec_.send(conn, pkt);
}