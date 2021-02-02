#include
"muduo/net/TcpServer.h"
#include
"muduo/net/Buffer.h"
#include
"muduo/net/EventLoop.h"
#include
"muduo/base/Logging.h"
#include
<iostream>
#include
<chrono>
#include
<iomanip>

using namespace std;

class DayTimeServer {
private:
muduo::net::TcpServer tcpServer;

static void onConnection(const muduo::net::TcpConnectionPtr &conn) {
if (conn->connected()) {
cout << "连接建立" << endl;
// 获取unix时间
auto time = chrono::system_clock::to_time_t(chrono::system_clock::now());
auto s = asctime(localtime(&time));
conn->send(string{ s });
} else {
cout << "连接断开" << endl;
}
}

static void
onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp timestamp) {
string msg(buf->retrieveAllAsString());
LOG_INFO << conn->name() << " discards " << msg.size()
<< " bytes received at " << timestamp.toString();
}

public:
DayTimeServer(muduo::net::EventLoop *loop, muduo::net::InetAddress address)
: tcpServer(loop, address, "DayTimeServer") {
tcpServer.setConnectionCallback(onConnection);
tcpServer.setMessageCallback(onMessage);

}

void start() {
tcpServer.start();
}
};

int main() {
muduo::net::EventLoop loop;
muduo::net::InetAddress address(7777);
DayTimeServer dayTimeServer(&loop, address);
dayTimeServer.start();
loop.loop();
return 0;
}