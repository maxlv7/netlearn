#include <iostream>
/*
 * 功能：丢弃所有收到的数据
 *
 * */
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"
#include <memory>
#include <unistd.h>

using namespace std;
using namespace std::placeholders;

class DiscardServer {
private:
    muduo::net::TcpServer tcpServer;

    // 这里就是accept成功的回调
    void onConnection(const muduo::net::TcpConnectionPtr &conn) {
        LOG_INFO << "onConnection";
    }

    // 这里可能是poll监听到socket可读的回调
    void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp time) {
        LOG_INFO << "onMessage";
    }

public:

    DiscardServer(muduo::net::EventLoop *Loop, muduo::net::InetAddress address)
            : tcpServer(Loop, address, "DiscardServer") {
        auto cc = bind(&DiscardServer::onConnection, this, placeholders::_1);
        auto mc = bind(&DiscardServer::onMessage, this, _1, _2, _3);
        tcpServer.setConnectionCallback(cc);
        tcpServer.setMessageCallback(mc);
    }

    void start() {
        tcpServer.start();
    }
};

int main() {
    LOG_INFO << "pid = " << getpid();
    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr{7777};
    DiscardServer server(&loop, listenAddr);
    server.start();
    loop.loop();

    return 0;
}
