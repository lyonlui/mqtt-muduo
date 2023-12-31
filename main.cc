#include "MqttServer.h"

using namespace mqtt;

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  argc = 2;
  if (argc > 1)
  {
    EventLoop loop;
    //uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    uint16_t port = static_cast<uint16_t>(1883);
    InetAddress serverAddr(port);
    MqttServer server(&loop, serverAddr);
    server.start();
    loop.loop();
  }
  else
  {
    printf("Usage: %s port\n", argv[0]);
  }
}