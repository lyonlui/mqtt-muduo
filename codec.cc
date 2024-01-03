#include "codec.h"

#include <string>


void MqttCodec::onMessage(const muduo::net::TcpConnectionPtr& conn,
                          muduo::net::Buffer* buf,
                          muduo::Timestamp receiveTime)
{
    while (buf->readableBytes() >= mqtt::MQTT_HEADER_LEN) // MQTT_HEADER_LEN == 2
    {

        
        uint32_t remaining_length = 0;
        int lshift = 0;
        uint8_t offset = 0;

        int8_t x;
        do
        {
            if(mqtt::MQTT_HEADER_LEN + offset > buf->readableBytes())
                return;
            offset++;
            if(offset > 4)
            {
                errorCallback_(conn, buf, receiveTime, kInvalidRemainingLen);
                return;
            }
            x = *(buf->peek() + offset);
            /* parse next byte*/
            remaining_length += (uint32_t) ((x & 0x7F) << lshift);
            lshift += 7;
        } while(x & 0x80);

        offset--;       //因为header中包含了一个remaining length的长度，所以，这里要减去 1
        size_t pktLen = static_cast<size_t>(remaining_length + mqtt::MQTT_HEADER_LEN + offset);
        if (pktLen > kMaxMessageLen || pktLen < kMinMessageLen)
        {
            errorCallback_(conn, buf, receiveTime, kInvalidLength);
            break;
        }
        else if (buf->readableBytes() >= pktLen)
        {
            ErrorCode errorCode = kNoError;
            mqtt::MqttPacketPtr message = parse(buf->peek(), pktLen, remaining_length ,&errorCode);
            
            if (errorCode == kNoError && message)
            {
                buf->retrieve(pktLen);
                messageCallback_(conn, message, receiveTime);
            }
            else
            {
                errorCallback_(conn, buf, receiveTime, errorCode);
                break;
            }
        }
        else
        {
            break;
        }

     
    }
}

namespace
{
  const std::string kNoErrorStr = "NoError";
  const std::string kInvalidLengthStr = "InvalidLength";
  const std::string kCheckSumErrorStr = "CheckSumError";
  const std::string kInvalidNameLenStr = "InvalidNameLen";
  const std::string kUnknownMessageTypeStr = "UnknownMessageType";
  const std::string kParseErrorStr = "ParseError";
  const std::string kUnknownErrorStr = "UnknownError";
}

const std::string& MqttCodec::errorCodeToString(ErrorCode errorCode)
{
  switch (errorCode)
  {
   case kNoError:
     return kNoErrorStr;
   case kInvalidLength:
     return kInvalidLengthStr;
   case kCheckSumError:
     return kCheckSumErrorStr;
   case kInvalidNameLen:
     return kInvalidNameLenStr;
   case kUnknownMessageType:
     return kUnknownMessageTypeStr;
   case kParseError:
     return kParseErrorStr;
   default:
     return kUnknownErrorStr;
  }
}

void MqttCodec::defaultErrorCallback(const muduo::net::TcpConnectionPtr& conn,
                                         muduo::net::Buffer* buf,
                                         muduo::Timestamp,
                                         ErrorCode errorCode)
{
    LOG_ERROR << "MqttCodec::defaultErrorCallback - " << errorCodeToString(errorCode);
    if (conn && conn->connected())
    {
        conn->shutdown();
    }
}

void MqttCodec::fillEmptyBuffer(muduo::net::Buffer* buf, const mqtt::MqttPacket& message, ErrorCode* errorCode)
{
    
    message.toFillBuffer(buf);
    int remain_lens = buf->readableBytes();
    //TODO 计算剩余长度
    if(remain_lens >= mqtt::MAX_REMAINING_LENGHT)
    {
        *errorCode = kInvalidLength;
        return;
    }

    uint8_t bytes[mqtt::MAX_LEN_BYTES] = {0};
    int btsz = -1;

    do {
        btsz++;
        /* consume byte and assert at least 1 byte left */
        if (btsz >= mqtt::MAX_LEN_BYTES)
        {
            *errorCode = kInvalidLength;
            return;
        }
        /* pack next byte */
        bytes[btsz] = remain_lens & 0x7F;
        if(remain_lens > 127) bytes[btsz] |= 0x80;
        remain_lens = remain_lens >> 7;
    } while(bytes[btsz] & 0x80);

    for(int i = btsz; i >= 0; i--)
    {
        buf->prependInt8(bytes[i]);
        if(bytes[i] & 0x80)
            continue;
        else
            break;
    }

    buf->prependInt8(message.getRawHeader());
    
}

mqtt::MqttPacket* MqttCodec::createMessage(const mqtt::mqtt_header& header)
{
    mqtt::MqttPacket* mqttpkt = nullptr;

    switch (header.bits.type)
    {
    case mqtt::CONNECT :
        mqttpkt = new mqtt::MqttConnect(header);
        break;
    case mqtt::CONNACK :
        mqttpkt = new mqtt::MqttConnack(header);
        break;
    case mqtt::SUBSCRIBE :
        mqttpkt = new mqtt::MqttSubscribe(header);
        break;
    case mqtt::UNSUBSCRIBE :
        mqttpkt = new mqtt::MqttUnsubscribe(header);
        break;
    case mqtt::SUBACK :
        mqttpkt = new mqtt::MqttSuback(header);
        break;
    case mqtt::PUBLISH :
        mqttpkt = new mqtt::MqttPublish(header);
        break;
    case mqtt::PUBACK :
    case mqtt::PUBREC :
    case mqtt::PUBREL :
    case mqtt::PUBCOMP:
    case mqtt::UNSUBACK :
        mqttpkt = new mqtt::MqttAck(header);
        break;
    case mqtt::PINGREQ :    
    case mqtt::PINGRESP :
    case mqtt::DISCONNECT :
        mqttpkt = new mqtt::MqttPacket(header);
        break;
    default:
        break;
    }

    return mqttpkt;
    
}

mqtt::MqttPacketPtr MqttCodec::parse(const char* buf, int len, int remaining_lens, ErrorCode* error)
{
    mqtt::MqttPacketPtr message;
    mqtt::mqtt_header header;
    header.byte = *buf;
    //buf++;

    message.reset(createMessage(header));
    if (message)
    {
        // parse from buffer
        const char* data = buf + (len - remaining_lens);
        if (message->ParseFromArray(data, remaining_lens))
        {
          *error = kNoError;
        }
        else
        {
          *error = kParseError;
        }
    }
    else
    {
        *error = kUnknownMessageType;
    }

    return message;
}