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
        handlePublish(conn, pkt, time);
        break;
    case mqtt::PUBACK :
        break;
    case mqtt::PUBREC :
        
        break;
    case mqtt::PUBREL :
        handlePubRel(conn, pkt, time);
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

void MqttPacketHandle::handleConnectError(const muduo::net::TcpConnectionPtr& conn, MqttConnack& ack, const uint8_t rtc)
{
    ConnAckFlag flag;
    flag.bits.session_present = false;
    ack.setConnAckFlag(flag);
    ack.setConnRetCode(rtc);
    send_(conn, ack);
    conn->shutdown();
}

void MqttPacketHandle::handleConnection(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt, muduo::Timestamp time)
{
    std::shared_ptr<MqttConnect> connPkt(pkt, static_cast<MqttConnect*>(pkt.get()));

    MqttConnack ack;

    if(!conn->getContext().empty())         //重复发送connect报文
    {
        defaultCallback_(conn, pkt, time);
        return;
    }
   


    if( connPkt->protocolName() != PROTOCOL_NAME)
    {
        defaultCallback_(conn, pkt, time);
        return;
    }
    
    if( connPkt->protocolVersion() != PROTOCOL_VERSION_v311)
    {
        handleConnectError(conn, ack, ConnReturnCode::VersionError);
        return;
    }

    if( connPkt->hasReserved())
    {
        defaultCallback_(conn, pkt, time);
        return;
    }
    
    std::string username = connPkt->userName();
    std::string password = connPkt->password();

    if( ( connPkt->hasUsername() && username.empty())   ||
        ( !connPkt->hasUsername() && !username.empty()) ||
        ( connPkt->hasPassword() && password.empty())   ||
        ( !connPkt->hasPassword() && !password.empty()) ||
        ( !connPkt->hasUsername() && connPkt->hasPassword()) ||
        ( username.empty() && !password.empty()))
    {
        defaultCallback_(conn, pkt, time);
        return;
    }

    std::string willTopic = connPkt->willTopic();
    std::string willMsg = connPkt->willMsg();
    uint8_t willQos = connPkt->willQos();
    bool willRetain = connPkt->willRetain();

    if( connPkt->hasWill())
    {
        //处理遗嘱消息
        if(willTopic.empty() || !validateUtf8(willTopic) || willMsg.empty())
        {
            defaultCallback_(conn, pkt, time);
            return;
        }
        if(willQos > qos_levle::EXACTLY_ONCE || willQos < qos_levle::AT_MOST_ONCE)
        {
            defaultCallback_(conn, pkt, time);
            return;
        }
    }
    else
    {
        if(willRetain || willQos || !willTopic.empty() || !willMsg.empty())
        {
            defaultCallback_(conn, pkt, time);
            return;
        }

    }
    
    std::string cid = connPkt->clientId();

    if(cid.empty())
    {
        if(connPkt->cleanSession())
        {
            //TODO 为client生成client ID
            return; 
        }
        else
        {
            handleConnectError(conn, ack, ConnReturnCode::ClientIDError);
            return;
        }
    }
    else
    {
       
        if(!validateUtf8(connPkt->clientId()))
        {
            defaultCallback_(conn, pkt, time);
            return;
        }
    }


    if(connPkt->hasUsername())
    {
        //TODO 判断用户名是否符合utf-8规范
        if(!validateUtf8(connPkt->userName()))
        {
            defaultCallback_(conn, pkt, time);
            return;
        }

        //TODO 验证用户
        if(false)   //验证失败
        {
            handleConnectError(conn, ack, ConnReturnCode::Unauthorized);
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

    if( connPkt->cleanSession())
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
            session = it->second;
        }
        else
        {
            std::lock_guard<std::mutex> lock(s_mutex_);
            sessions_[cid] = session;
        }
    }
    

    //TODO 处理keepalive为零的情况
    session->keepalive = connPkt->keeplive();

    
    if(session->hasWill() || connPkt->hasWill())
    {
        session->setWill(connPkt->hasWill());
        session->willRetain_ = willRetain;
        session->willQos_ = willQos;
        session->willTopic_ = std::move(willTopic);
        session->willMsg_ = std::move(willMsg);
    }


    ConnAckFlag flag;
    flag.bits.session_present = (connPkt->cleanSession() == false) 
                                    && (it != sessions_.end()); // 

    
    conn->setContext(cid);
    session->setState(Session::State::CONNECTED);

    ack.setConnAckFlag(flag);
    ack.setConnRetCode(ConnReturnCode::ConnAccept);
    send_(conn, ack);

    //session->lastMsgOut = muduo::Timestamp::now();
    
    

    //TODO 设置保持连接定时器
    
}

void MqttPacketHandle::handlePublish(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt, muduo::Timestamp time)
{

    if(pkt->qos() == qos_levle::AT_MOST_ONCE && pkt->isDup())
    {
        defaultCallback_(conn, pkt, time);
        return;
    }

    std::shared_ptr<MqttPublish> pubPkt(pkt, static_cast<MqttPublish*>(pkt.get()));

    std::string topic = pubPkt->topic();
    std::string payload = pubPkt->payload();
    uint8_t qos = pubPkt->qos();

    if(!validateUtf8(topic))
    {
        defaultCallback_(conn, pkt, time);
        return;
    }

    
    switch (qos)
    {
    case qos_levle::AT_MOST_ONCE:
        //TODO 处理qos：0的publish报文

        break;
    case qos_levle::AT_LEAST_ONCE:
        {
        //TODO 处理qos：1的publish报文

        //puback
        MqttPuback puback(makeHeader(packet_type::PUBACK));
        puback.setPktId(pubPkt->pktId());
        send_(conn, puback);
        break; 
        }
    case qos_levle::EXACTLY_ONCE:
        {
        //TODO 处理qos：2的publish报文

        MqttPubrec pubRec(makeHeader(packet_type::PUBREC));
        pubRec.setPktId(pubPkt->pktId());
        send_(conn, pubRec);
        break;
        }
    default:
        defaultCallback_(conn, pkt, time);
        break;
    }

    
}


void MqttPacketHandle::handlePubRel(const muduo::net::TcpConnectionPtr& conn,
                       const mqtt::MqttPacketPtr& pkt, muduo::Timestamp time)
{
    std::shared_ptr<MqttPubrel> pubRelPkt(pkt, static_cast<MqttPubrel*>(pkt.get()));
    if((pubRelPkt->getRawHeader() & 0x0F) != 0x02)
    {
        defaultCallback_(conn, pkt, time);
        return;
    }

    MqttPubcomp pubComp(makeHeader(packet_type::PUBCOMP));
    pubComp.setPktId(pubRelPkt->pktId());
    send_(conn, pubComp);

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

    // 必须丢弃任何与当前连接关联的未发布的遗嘱消息，具体描述见 3.1.2.5节 [MQTT-3.14.4-3]。
    if(sessions_[cid]->hasWill())
    {
        sessions_[cid]->setWill(false);
        sessions_[cid]->willRetain_ = 0;
        sessions_[cid]->willTopic_ = "";
        sessions_[cid]->willQos_ = 0;
        sessions_[cid]->willMsg_ = "";
    }
    


    // 清理session之后，客户端还没关闭连接的话，由服务器端关闭
    if(conn->connected())
        conn->shutdown();
}  






bool MqttPacketHandle::validateUtf8(const std::string& str)
{
    int codelen;
	int codepoint;

    if(str.empty()) return false;
    size_t len = str.size();
	if(len < 0 || len > 65536) return false;

	for(int i=0; i < len; i++){

		if(str[i] == 0){
			return false;
		}else if(str[i] <= 0x7f){
			codelen = 1;
			codepoint = str[i];
		}else if((str[i] & 0xE0) == 0xC0){
			/* 110xxxxx - 2 byte sequence */
			if(str[i] == 0xC0 || str[i] == 0xC1){
				/* Invalid bytes */
				return false;
			}
			codelen = 2;
			codepoint = (str[i] & 0x1F);
		}else if((str[i] & 0xF0) == 0xE0){
			/* 1110xxxx - 3 byte sequence */
			codelen = 3;
			codepoint = (str[i] & 0x0F);
		}else if((str[i] & 0xF8) == 0xF0){
			/* 11110xxx - 4 byte sequence */
			if(str[i] > 0xF4){
				/* Invalid, this would produce values > 0x10FFFF. */
				return false;
			}
			codelen = 4;
			codepoint = (str[i] & 0x07);
		}else{
			/* Unexpected continuation byte. */
			return false;
		}

		/* Reconstruct full code point */
		if(i == len-codelen+1){
			/* Not enough data */
			return false;
		}
		for(int  j=0; j< codelen-1; j++){
			if((str[++i] & 0xC0) != 0x80){
				/* Not a continuation byte */
				return false;
			}
			codepoint = (codepoint<<6) | (str[i] & 0x3F);
		}

		/* Check for UTF-16 high/low surrogates */
		if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
			return false;
		}

		/* Check for overlong or out of range encodings */
		/* Checking codelen == 2 isn't necessary here, because it is already
		 * covered above in the C0 and C1 checks.
		 * if(codelen == 2 && codepoint < 0x0080){
		 *	 return false;
		 * }else
		*/
		if(codelen == 3 && codepoint < 0x0800){
			return false;
		}else if(codelen == 4 && (codepoint < 0x10000 || codepoint > 0x10FFFF)){
			return false;
		}

		/* Check for non-characters */
		if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
			return false;
		}
		if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
			return false;
		}
		/* Check for control characters */
		if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
			return false;
		}
	}
	return true;
}