cmake_minimum_required(VERSION 3.0)

project(mqtt CXX)

include_directories("muduo")

add_executable(mqtt main.cc MqttServer.cc MqttPacketHandle.cc codec.cc)
target_link_libraries(mqtt muduo_net)
target_link_libraries(mqtt muduo_base)
target_link_libraries(mqtt pthread rt)