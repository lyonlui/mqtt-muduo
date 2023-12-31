#pragma once

#include <cstdint>
#include <string>

namespace mqtt{

static constexpr const char* PROTOCOL_NAME = "MQTT";
static constexpr uint16_t PROTOCOL_NAME_LEN = 4;
static constexpr uint8_t PROTOCOL_VERSION_v311 = 4;
static constexpr int MAX_REMAINING_LENGHT = 256*1024*1024;

static constexpr uint8_t MQTT_HEADER_LEN = 2;
static constexpr uint8_t MQTT_ACK_LEN = 4; 
static constexpr uint8_t MAX_LEN_BYTES = 4;
                                        
static constexpr uint8_t CONNACK_BYTE       = 0x20;       
static constexpr uint8_t PUBLISH_BYTE       = 0x30; 
static constexpr uint8_t PUBACK_BYTE        = 0x40; 
static constexpr uint8_t PUBREC_BYTE        = 0x50;
static constexpr uint8_t PUBREL_BYTE        = 0x60;
static constexpr uint8_t PUBCOMP_BYTE       = 0x70;
static constexpr uint8_t SUBSCRIBE_BYTE     = 0x80;
static constexpr uint8_t SUBACK_BYTE        = 0x90;
static constexpr uint8_t UNSUBSCRIBE_BYTE   = 0xA0;
static constexpr uint8_t UNSUBACK_BYTE      = 0xB0;
static constexpr uint8_t PINGREQ_BYTE       = 0xC0;
static constexpr uint8_t PINGRESP_BYTE      = 0xD0;
static constexpr uint8_t DISCONNECT_BYTE    = 0xE0;

/*     name      value       报文流动方向                描述             */
/*     CONNACK   2           clinet->broker             连接报文确认     */
/*     PUBLISH   3           clinet<->broker            发布消息        */
/*     PUBACK    4           clinet<->broker            QoS        */

enum packet_type {
    CONNECT     = 1,
    CONNACK     = 2,
    PUBLISH     = 3,
    PUBACK      = 4,
    PUBREC      = 5,
    PUBREL      = 6,
    PUBCOMP     = 7,
    SUBSCRIBE   = 8,
    SUBACK      = 9,
    UNSUBSCRIBE = 10,
    UNSUBACK    = 11,
    PINGREQ     = 12,
    PINGRESP    = 13,
    DISCONNECT  = 14,
};

static constexpr const char* TypeToString(packet_type e)
{
    switch (e)
    {
        case packet_type::CONNECT: return "CONNECT";
        case packet_type::CONNACK: return "CONNACK";
        case packet_type::PUBLISH: return "PUBLISH";
        case packet_type::PUBACK: return "PUBACK";
        case packet_type::PUBREC: return "PUBREC";
        case packet_type::PUBREL: return "PUBREL";
        case packet_type::PUBCOMP: return "PUBCOMP";
        case packet_type::SUBSCRIBE: return "SUBSCRIBE";
        case packet_type::SUBACK: return "SUBACK";
        case packet_type::UNSUBSCRIBE: return "UNSUBSCRIBE";
        case packet_type::UNSUBACK: return "UNSUBACK";
        case packet_type::PINGREQ: return "PINGREQ";
        case packet_type::PINGRESP: return "PINGRESP";
        case packet_type::DISCONNECT: return "DISCONNECT";
        default: return "UNDEFINE TYPE";
    }
}

enum qos_levle { AT_MOST_ONCE, AT_LEAST_ONCE, EXACTLY_ONCE };

enum sub_ack { 
    QoS0    = 0x00, 
    QoS1    = 0x01,
    QoS2    = 0x02,
    FAILURE = 0x80,  
};

enum ConnReturnCode 
{
    ConnAccept          = 0,
    VersionError,
    ClientIDError,
    ServerError,
    InvalidUser,
    Unauthorized,            
};

union mqtt_header
{
    unsigned char byte;
    struct {
        unsigned retain : 1;
        unsigned qos    : 2;
        unsigned dup    : 1;
        unsigned type   : 4;
    } bits;
};

union ConnAckFlag
{
    unsigned char byte;
    struct {
        unsigned session_present    : 1;
        unsigned reserved           : 7;
    } bits;
};

struct SubTuple
{
    unsigned short topic_len;
    std::string topic;
    unsigned qos;
};

struct UnsubTuple
{
    unsigned short topic_len;
    std::string topic;
};

union ConnFlag {
    unsigned char byte;
    struct {
        int reserved            : 1;
        unsigned clean_session  : 1;
        unsigned will           : 1;
        unsigned will_qos       : 2;
        unsigned will_retain    : 1;
        unsigned password       : 1;
        unsigned username       : 1;
    } bits;
};

}