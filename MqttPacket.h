#pragma once

#include "MqttProtocol.h"


#include <sstream>
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <memory>

namespace mqtt{


class MqttPacket
{

public: 
    MqttPacket();
    MqttPacket(mqtt_header hdr)
                : header_(hdr)
    {
    }

    virtual ~MqttPacket(){}

    
    packet_type getType() const { return packet_type(header_.bits.type); }

    //unsigned char getHeader { return header_.byte; }
    
    
    virtual std::string toString() const
    {
        char buf[256];
        snprintf(buf, sizeof buf , 
                "header => {\"MsgType\" : %s, \"Dup\" : %d, \"Qos\" : %d, \"Retain\" : %d } ", 
                TypeToString(packet_type(header_.bits.type)),
                header_.bits.dup,
                header_.bits.qos,
                header_.bits.retain);
        return std::string(buf);
    }

    virtual bool ParseFromArray(const char* data, size_t remain_lens)
    {
        return remain_lens == 0;
    }
    virtual void toFillBuffer(muduo::net::Buffer* buf) const
    {
        return ;
    }

    uint8_t getHederByte() const {return header_.byte;}
protected:
    mqtt_header header_;

};

using MqttPingReq      = MqttPacket;
using MqttPingResp     = MqttPacket;
using MqttDisconnect   = MqttPacket;

class MqttAck : public MqttPacket
{
public:
    MqttAck(mqtt_header hdr)
            : MqttPacket(hdr)
    {   
    } 
    ~MqttAck(){}

    std::string toString() const override
    {
        std::string str = MqttPacket::toString();
        char buf[256];
        snprintf(buf, sizeof buf, "Package ID => {\"packageID\" : %hx} ", pktId_);
        return str + std::string(buf);
    }

    bool ParseFromArray(const char* data, size_t remain_lens) override
    {

        muduo::net::Buffer buf;
        buf.append(data, remain_lens);
        if(buf.readableBytes() != sizeof(int16_t))
            return false;
        this->pktId_ = buf.readInt16();
    
        return true; 
    }

    void toFillBuffer(muduo::net::Buffer* buf) const  override
    {
        buf->appendInt16(pktId_);
        buf->toStringPiece();
    }

private:
    unsigned short pktId_;

};


using MqttPuback       = MqttAck;
using MqttPubrec       = MqttAck;
using MqttPubrel       = MqttAck;
using MqttPubcomp      = MqttAck;
using MqttUnsuback     = MqttAck;

class MqttConnect : public MqttPacket
{


public:
    MqttConnect(mqtt_header hdr)
                : MqttPacket(hdr),
                  keeplive_(0),
                  clientId_(""),
                  userName_(""),
                  password_(""),
                  willTopic_(""),
                  willMessage_(""),
                  protocolName_(""),
                  protocolVesrsion_(0)
    {  
    } 
    ~MqttConnect(){}

    std::string toString() const override
    {
        std::stringstream ss;
        ss << MqttPacket::toString() << "Keeplive => " << keeplive_ << 
            ", ClientID => " << clientId_ <<
            ", UserName => " << userName_ <<
            ", Password => " << password_ <<
            ", WillTopic => " << willTopic_ <<
            ", WillMessage => " << willMessage_ <<
            ", ProtocolNmae => " << protocolName_ <<
            ", protocolVersion => " << protocolVesrsion_ << " ";
        return ss.str();
    }

    bool ParseFromArray(const char* data, size_t remain_lens) override
    {
        muduo::net::Buffer buf;
        buf.append(data, remain_lens);
        int16_t name_lens = buf.readInt16();

        if(name_lens != mqtt::PROTOCOL_NAME_LEN)
            return false;
/*
        if(mqtt::PROTOCOL_NAME != buf.retrieveAsString(name_lens))
            return false;
        
        if(mqtt::PROTOCOL_VERSION_v311 != buf.readInt8())
            return false;
*/
        protocolName_ = buf.retrieveAsString(name_lens);
        protocolVesrsion_ = buf.readInt8();

        /*Read variable header byte flags*/
        flag_.byte = buf.readInt8();

        /*Read keeplive MSB and LSB (2 bytes)*/
        keeplive_ = buf.readInt16();

        /*Read CID*/
        uint16_t cid_len = buf.readInt16();

        if(cid_len > 0)
        {
            clientId_ = buf.retrieveAsString(cid_len);
        }

        /*Read the Will topic and message if Will it set on flags*/
        if(flag_.bits.will)
        {
            uint16_t will_topic_len = buf.readInt16();
            willTopic_ = buf.retrieveAsString(will_topic_len);
            uint16_t will_msg_len = buf.readInt16();
            willMessage_ = buf.retrieveAsString(will_msg_len);
        }

        /*Read username if username flag is set */
        if(flag_.bits.username)
        {
            uint16_t username_len = buf.readInt16();
            userName_ = buf.retrieveAsString(username_len);
        }

        /*Read password if password flag is set */
        if(flag_.bits.password)
        {
            uint16_t pwd_len = buf.readInt16();
            password_ = buf.retrieveAsString(pwd_len);
        }
        

        return true; 
    }
    /*
    void toFillBuffer(muduo::net::Buffer* buf) const override
    {
        return "";
    }
    */

private:
    ConnFlag flag_;
    unsigned short keeplive_;
    std::string clientId_;
    std::string userName_;
    std::string password_;
    std::string willTopic_;
    std::string willMessage_;

    std::string protocolName_;
    unsigned short protocolVesrsion_;

};


class MqttConnack : public MqttPacket
{



public:
    MqttConnack();
    MqttConnack(mqtt_header hdr)
                : MqttPacket(hdr)
    {  
    }
    MqttConnack(mqtt_header hdr, u_char rc, ConnAckFlag flag)
                : MqttPacket(hdr),
                  connRetCode_(rc),
                  connAckFlag_(flag)
    {  
    }

    ~MqttConnack(){}

    
    std::string toString() const override
    {
        std::string str = MqttPacket::toString();
        char buf[256];
        snprintf(buf, sizeof buf, "payload => {\"SessionPresent\": %d, \"ReturnCode\": %d} ", connAckFlag_.bits.session_present, connRetCode_);
        return str + buf;
    }

    bool ParseFromArray(const char* data, size_t remain_lens) override
    {
        muduo::net::Buffer buf;
        buf.append(data, remain_lens);

        connAckFlag_.byte = buf.readInt8();
        connRetCode_ = buf.readInt8();

        return true;
    }

    void toFillBuffer(muduo::net::Buffer* buf) const override
    {
        
        buf->appendInt8(connAckFlag_.byte);
        buf->appendInt8(connRetCode_);
    }

private:
    unsigned char connRetCode_;
    ConnAckFlag connAckFlag_;
};

class MqttSubscribe : public MqttPacket
{



public:
    MqttSubscribe();
    MqttSubscribe(mqtt_header hdr)
                    : MqttPacket(hdr)
    {   
    }

    ~MqttSubscribe(){}


    
    std::string toString() const override
    {
        std::string str = MqttPacket::toString();
        char buf[256];
        snprintf(buf, sizeof buf, "Package ID => {\"packageID\" : %hx} ", pktId_);
        std::stringstream ss;
        ss << (str + buf);
        for(auto& t : tuples_)
        {
            ss << tupleToString(t);
        }
        return ss.str();
    }

    bool ParseFromArray(const char* data, size_t remain_lens) override
    {
        muduo::net::Buffer buf;
        buf.append(data, remain_lens);
        pktId_ = buf.readInt16();

        while(buf.readableBytes() > 0)
        {
            SubTuple t;
            t.topic_len = buf.readInt16();
            t.topic = buf.retrieveAsString(t.topic_len);
            t.qos = buf.readInt8();
            tuples_.push_back(std::move(t));
        }

        return true;
    }

/*
    void toFillBuffer(muduo::net::Buffer* buf) const
    {
        return "";
    }
*/

private:
    unsigned short pktId_;
    std::vector<SubTuple> tuples_;
    std::string tupleToString(const SubTuple& tuple) const
    {
        return "[\"Topic\" :  " + tuple.topic + 
                ", \"TopicLen\" : " + std::to_string(tuple.topic_len) +  
                ", \"Qos\" : " + std::to_string(tuple.qos) +
                "]";
    }
};

class MqttUnsubscribe : public MqttPacket
{



public:
    MqttUnsubscribe();
    MqttUnsubscribe(mqtt_header hdr)
                    : MqttPacket(hdr)
    {
    }

    ~MqttUnsubscribe(){}

    
    std::string toString() const override
    {
        std::string str = MqttPacket::toString();
        char buf[256];
        snprintf(buf, sizeof buf, "Package ID => {\"packageID\" : %hx} ", pktId_);
        std::stringstream ss;
        ss << (str + buf);
        for(auto& t : tuples_)
        {
            ss << tupleToString(t);
        }
        return ss.str();
    }

    bool ParseFromArray(const char* data, size_t remain_lens) override
    {
        muduo::net::Buffer buf;
        buf.append(data, remain_lens);
        pktId_ = buf.readInt16();

        while(buf.readableBytes() > 0)
        {
            UnsubTuple t;
            t.topic_len = buf.readInt16();
            t.topic = buf.retrieveAsString(t.topic_len);
            tuples_.push_back(std::move(t));
        }

        return true;
    }
/*
    void toFillBuffer(muduo::net::Buffer* buf) const
    {
        return "";
    }
*/

private:
    unsigned short pktId_;
    std::vector<UnsubTuple> tuples_;
    std::string tupleToString(const UnsubTuple& tuple) const
    {
        return "[\"Topic\" :  " + tuple.topic + 
                ", \"TopicLen\" : " + std::to_string(tuple.topic_len) +
                "]";
    }
    
};


class MqttSuback : public MqttPacket
{
public:
    MqttSuback();
    MqttSuback(mqtt_header hdr)
                : MqttPacket(hdr)
    {    
    }

    ~MqttSuback(){}
    
    std::string toString() const override
    {
        std::string str = MqttPacket::toString();
        return str;
    }

    bool ParseFromArray(const char* data, size_t remain_lens) override
    {
        muduo::net::Buffer buf;
        buf.append(data, remain_lens);
        pktId_ = buf.readInt16();

        while(buf.readableBytes() > 0)
        {
            uint8_t rcs = buf.readInt8();
            retCodeSub_.push_back(rcs);
        }

        return true;
    }

    void toFillBuffer(muduo::net::Buffer* buf) const override
    {
        
        buf->appendInt16(pktId_);
        for(auto& rc : retCodeSub_)
            buf->appendInt8(rc);
    }

private:
    unsigned short pktId_;
    std::vector<u_char> retCodeSub_;
    
};

class MqttPublish : public MqttPacket
{
public:
    MqttPublish();
    MqttPublish(mqtt_header hdr)
                : MqttPacket(hdr)
    { 
    }

    ~MqttPublish(){}

    
    std::string toString() const override
    {
        std::string str = MqttPacket::toString();
        std::stringstream ss;
        ss << str << "Package ID => {\"packageID\" : "<< pktId_ <<"} ";
        ss << "Topic => " << topic_ << ", Payload => " << payload_ << " ";
        return ss.str();
    }

    bool ParseFromArray(const char* data, size_t remain_lens)   override
    {
        muduo::net::Buffer buf;
        buf.append(data, remain_lens);
        topicLen_ = buf.readInt16();
        topic_ = buf.retrieveAsString(topicLen_);

        if(header_.bits.qos > mqtt::AT_MOST_ONCE)
        {
            pktId_ = buf.readInt16();
        }

        payload_ = buf.retrieveAllAsString();

        return true;
    }

    void toFillBuffer(muduo::net::Buffer* buf) const override
    {
        
        buf->appendInt16(static_cast<int16_t>(topic_.size()));
        buf->append(topic_.data(), topic_.size());
        
        if(header_.bits.qos > mqtt::AT_MOST_ONCE)
        {
            buf->appendInt16(pktId_);
        }

    }

private:
    unsigned short pktId_;
    std::string topic_;
    unsigned short topicLen_;
    std::string payload_;
    
};

using MqttPacketPtr = std::shared_ptr<mqtt::MqttPacket>;

static mqtt_header makeHeader(packet_type type)
{
    mqtt_header hdr;
    hdr.bits.type = type;
    return hdr;
}

}

