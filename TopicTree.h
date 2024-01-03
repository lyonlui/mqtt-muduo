#pragma once;

#include <muduo/base/noncopyable.h>
#include <unordered_map>
#include <string>
#include <list>

namespace mqtt
{

    
struct Topic
{
    std::list<std::string> subscribers_;        //订阅者们
    std::string owner_;                         //topic所有者
    bool retain_;
    std::pair<unsigned, std::string> retainPayload_;    //保留消息
    std::pair<unsigned, std::string> payload_;
      
    uint16_t pktid_;
};


class TopicTree : muduo::noncopyable
{
public:
    TopicTree();
    ~TopicTree();

    void addTopic(const std::string topicName, const Topic& topic);

private:
    using TopicMap = std::unordered_map<std::string, Topic&>;
    TopicMap topics_;

};

}