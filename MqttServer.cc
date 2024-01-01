#include "MqttServer.h"

#include <set>
#include <stdio.h>
#include <unistd.h>



using namespace muduo;
using namespace muduo::net;

  
using namespace mqtt;


MqttServer::MqttServer(EventLoop* loop,
             const InetAddress& listenAddr)
  : loop_(loop),
    server_(loop, listenAddr, "MqttServer"),
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
  

  if(conn->connected()) //处理新连接
  {
    LOG_INFO << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is UP";
    

    // 3s内，客户端还没有发送connect报文，完成connect， 判定为无效连接，直接踢出
    
    loop_->runAfter(3.0, [conn]()
    {
      if(conn->connected() && conn->getContext().empty())
      {
        LOG_INFO << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " Invild Connect";
        conn->shutdown();
      }

    });  

  }
  else //处理断开的连接
  {
    LOG_INFO << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is DOWN";
    

  }
  
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