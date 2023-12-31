#pragma once

#include <muduo/base/noncopyable.h>
#include <muduo/net/TcpConnection.h> 

#include <memory>


namespace mqtt
{

class MqttServer;

class Session : public std::enable_shared_from_this<Session>,
                muduo::noncopyable
{
public:
    enum State
    {
        CONNECTING,
        CONNECTED,
        DISCONNECT,
    };

    Session(const muduo::net::TcpConnectionPtr& conn)
        : conn_(conn),
          state_(CONNECTING),
          hasWill_(false)
    {
    }

    Session(MqttServer* owner, const muduo::net::TcpConnectionPtr& conn)
        : owner_(owner),
          conn_(conn),
          state_(CONNECTING)
    {
    }
    
    State state() { return state_;}
    void setState(State state) { state_ = state;}
    void setTcpConn(muduo::net::TcpConnectionPtr conn) { conn_ = conn;} 
    void shutTcpConn() 
    { 
        if(conn_->connected())
            conn_->shutdown();
    }

    bool hasWill() { return hasWill_;}
    void setWill(bool hasWill) { hasWill_ = hasWill;}

    muduo::Timestamp lastPing;
    muduo::Timestamp lastMsgIn;
    muduo::Timestamp lastMsgOut;
    uint16_t keepalive;

    std::string willTopic_;
    std::string willMsg_;
    bool willRetain_;
    uint8_t willQos_;
    
private:
    State state_;
    MqttServer* owner_;
    muduo::net::TcpConnectionPtr conn_;
    bool hasWill_;
};

using SessionPtr = std::shared_ptr<Session>;
using WeakSessionPtr = std::weak_ptr<Session>;


} // namespace mqtt