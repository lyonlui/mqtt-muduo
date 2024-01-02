#include "MqttPacketHandle.h"

using namespace mqtt;

void MqttPacketHandle::onMqttMessage(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt,
                       muduo::Timestamp time)
{

    LOG_INFO << "MqttPacketHandle:onMqttMessage " << pkt->toString();

    //获取当前连接的session
   
    if(conn->getContext().empty() && pkt->getType() != mqtt::CONNECT)
    {
        defaultCallback_(conn, pkt, time);
    }

    switch (pkt->getType())
    {
    case mqtt::CONNECT :
        handleConnection(conn, pkt, time);
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
        handlePingRequest(conn, pkt, time);  
        break;
    case mqtt::DISCONNECT :
        handleDisconnect(conn, pkt, time);
        break;
    default:        //unknow packet
        defaultCallback_(conn, pkt, time);
        break;
    }
    
}

void MqttPacketHandle::handleConnection(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt, muduo::Timestamp time)
{
    std::shared_ptr<MqttConnect> connPkt(pkt, static_cast<MqttConnect*>(pkt.get()));

    ConnAckFlag flag;
    MqttConnack ack(makeHeader(packet_type::CONNACK));
    u_char rtc;

    if(!conn->getContext().empty())         //重复发送connect报文
    {
        defaultCallback_(conn, pkt, time);
    }
   


    if( connPkt->protocolName() != PROTOCOL_NAME)
    {
        defaultCallback_(conn, pkt, time);
        return;
    }
    
    if( connPkt->protocolVersion() != PROTOCOL_VERSION_v311)
    {
        
        flag.bits.session_present = false;
        rtc = ConnReturnCode::VersionError;
        ack.setConnAckFlag(flag);
        ack.setConnRetCode(rtc);
        send_(conn, ack);
        conn->shutdown();
        return;
    }

    if( connPkt->connFlag().bits.reserved)
    {
        defaultCallback_(conn, pkt, time);
        return;
    }
    

    if( ( connPkt->connFlag().bits.username && connPkt->userName().empty())   ||
        ( !connPkt->connFlag().bits.username && !connPkt->userName().empty()) ||
        ( connPkt->connFlag().bits.password && connPkt->password().empty())   ||
        ( !connPkt->connFlag().bits.password && !connPkt->password().empty()) ||
        ( !connPkt->connFlag().bits.username && connPkt->connFlag().bits.password))
    {
        defaultCallback_(conn, pkt, time);
        return;
    }

    if(connPkt->connFlag().bits.username)
    {
        //TODO 判断用户名是否符合utf-8规范

        //TODO 验证用户
        if(false)   //验证失败
        {
            flag.bits.session_present = false;
            rtc = ConnReturnCode::Unauthorized;
            ack.setConnAckFlag(flag);
            ack.setConnRetCode(rtc);
            send_(conn, ack);
            return;
        }
    }


    std::string cid = connPkt->clientId();

    if(cid.empty())
    {
        if(connPkt->connFlag().bits.clean_session)
        {
            //TODO 为client生成client ID
            return; 
        }
        else
        {

            flag.bits.session_present = false;
            rtc = ConnReturnCode::ClientIDError;
            ack.setConnAckFlag(flag);
            ack.setConnRetCode(rtc);
            send_(conn, ack);
            conn->shutdown();
            return;
        }
    }

    SessionPtr session = std::make_shared<Session>(conn);

    
    auto it = sessions_.end();

    {
        std::lock_guard<std::mutex> lock(s_mutex_);
        it = sessions_.find(cid);
    }

    if(it != sessions_.end())
    {
        it->second->shutTcpConn();      //踢掉其它相同cid的客户端
    }

    if( connPkt->connFlag().bits.clean_session)
    {

        //清理session
        {
            std::lock_guard<std::mutex> lock(s_mutex_);
            sessions_[cid] = session;
        }
    }
    else
    {
        //恢复session
        if(it != sessions_.end())
        {
            it->second->setTcpConn(conn);
            WeakSessionPtr weakSession(sessions_[cid]);
            conn->setContext(weakSession);
            session = it->second;
        }
        else
        {
            std::lock_guard<std::mutex> lock(s_mutex_);
            sessions_[cid] = session;
        }
    }

    if( connPkt->connFlag().bits.will_retain)
    {
        //处理遗嘱消息
    }
    

    //TODO 处理keepalive为零的情况
    session->keepalive = connPkt->keeplive();


    flag.bits.session_present = (connPkt->connFlag().bits.clean_session == false) 
                                    && (it != sessions_.end()); // 

    
    conn->setContext(cid);
    session->setState(Session::State::CONNECTED);

    rtc = ConnReturnCode::ConnAccept;
    ack.setConnAckFlag(flag);
    ack.setConnRetCode(rtc);
    send_(conn, ack);

    //session->lastMsgOut = muduo::Timestamp::now();
    
    

    //TODO 设置保持连接定时器
    
}




void MqttPacketHandle::handlePingRequest(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt, muduo::Timestamp time)
{
    std::string cid = boost::any_cast<std::string>(conn->getContext());
    sessions_[cid]->lastPing = time;

    MqttPingResp resp(makeHeader(packet_type::PINGRESP));
    send_(conn, resp);
    sessions_[cid]->lastMsgOut = muduo::Timestamp::now();

    //TODO 更新保持连接定时器
}



void MqttPacketHandle::handleDisconnect(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt, muduo::Timestamp time)
{

    //TODO 清理会话 session

    std::string cid = boost::any_cast<std::string>(conn->getContext());

    sessions_[cid]->setState(Session::State::DISCONNECT);

    //TODO 必须丢弃任何与当前连接关联的未发布的遗嘱消息，具体描述见 3.1.2.5节 [MQTT-3.14.4-3]。
    

    // 清理session之后，客户端还没关闭连接的话，由服务器端关闭
    if(conn->connected())
        conn->shutdown();
}                       