#include <iostream>
#include <functional>
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/Logging.h"
#include <string>

#include <unistd.h>

using namespace std;
using namespace std::placeholders;

class EchoServer {
private:
    muduo::net::TcpServer tcpServer;

    void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp timestamp) {
        LOG_INFO << "onMessage";
        // 从buffer中收取所有数据
        string msg{buffer->retrieveAllAsString()};
        LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
                 << "data received at: " << timestamp.toString();
        conn->send(msg);
    }

    void onConnection(const muduo::net::TcpConnectionPtr &conn) {
        LOG_INFO << "onConnection";
        LOG_INFO << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort()
                 // 判断是刚连接还是断开连接
                 << "[" << (conn->connected() ? "UP" : "DOWN") << "]";
    }

public:
    EchoServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &inetAddress)
            : tcpServer(loop, inetAddress, "EchoServer") {
        tcpServer.setMessageCallback(bind(&EchoServer::onMessage, this, _1, _2, _3));
        tcpServer.setConnectionCallback(bind(&EchoServer::onConnection, this, _1));

    }

    void start() {
        tcpServer.start();
    }
};

int main() {
    LOG_INFO << "pid :" << getpid();
    muduo::net::EventLoop loop;
    muduo::net::InetAddress inetAddress(7777);
    EchoServer server{&loop, inetAddress};
    server.start();
    loop.loop();
    return 0;
}
