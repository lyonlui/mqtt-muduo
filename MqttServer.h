#pragma once

#include "codec.h"
#include "MqttPacketHandle.h"


#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Thread.h>
#include <muduo/net/InetAddress.h>
#include <mutex>

class Session;

using namespace muduo;
using namespace muduo::net;

namespace mqtt
{
class MqttServer : muduo::noncopyable
{
public:
    MqttServer(EventLoop* loop,
             const InetAddress& listenAddr);

    ~MqttServer(){}

    void setThreadNum(int threads) { server_.setThreadNum(threads); }
    void start();
    void stop();

private:
    EventLoop *loop_;
    TcpServer server_;

    MqttCodec codec_;
    MqttPacketHandle packetHandle_;

    std::mutex mutex_;
    //mutable muduo::MutexLock mutex_;
    //std::unordered_map<string, MqttSessionPtr> sessions_ GUARDED_BY(mutex_);
    void onConnection(const TcpConnectionPtr& conn);
    void send(const muduo::net::TcpConnectionPtr& conn,
            const mqtt::MqttPacket& pkt);
    void onUnknownMessage(const TcpConnectionPtr& conn,
                      const MqttPacketPtr& message,
                      Timestamp);
};
}