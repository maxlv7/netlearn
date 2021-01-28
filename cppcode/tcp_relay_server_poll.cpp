#include <iostream>
#include "common.hpp"
#include <poll.h>
#include <string>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <vector>
/*
 * 学习poll的使用
 * */
using namespace std;

//tcp连接信息
struct TcPConnection {
    int fd;
    vector<char> read_buffer;
    vector<char> write_buffer;
};

//存放客户端与服务端的tcp连接信息
struct PairTcPConnection {
    TcPConnection *local;
    TcPConnection *remote;
};

vector<TcPConnection *> channel;
vector<PairTcPConnection *> tcp_pair;

TcPConnection *getTcPConnection(int fd) {
    for (auto *s:channel) {
        if (s->fd == fd) {
            return s;
        }
    }
    return nullptr;
}

int FreeTcPConnection(int fd) {
    for (int i = 0; i < channel.size(); ++i) {
        if (channel[i]->fd == fd) {
            delete channel[i];
            channel.erase(channel.begin() + i);
            return 0;
        }
    }
    return -1;
}


int main() {
    auto listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server{};
    sockaddr_in client{};
    sockaddr_in remote{};
    // 设置套接字为非阻塞
    auto flags_server = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, flags_server | O_NONBLOCK);
    server.sin_port = htons(7777);
    server.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &server.sin_addr);
    auto status = bind(listen_fd, (sockaddr *) &server, sizeof(server));
    if (status < 0) {
        cout << "bind error!" << endl;
        cout << strerror(errno) << endl;
        exit(0);
    }
    listen(listen_fd, 10);
    //会自动初始化为0
    pollfd connection[1024]{0};
    for (auto &c:connection) {
        c.fd = -1;
    }
    connection[0].fd = listen_fd;
    connection[0].events = POLLIN;
    auto max_i = 0;
    auto ready_i = max_i;
    while (1) {
        auto ret = poll(connection, max_i + 1, -1);
        if (ret < 0) {
            cout << strerror(errno) << endl;
        }
        cout << "ok" << endl;
        //监听套接字可读,那么有新连接到来
        if (connection[0].revents == POLLIN) {
            unsigned int len = sizeof(client);
            auto new_fd = accept(connection[0].fd, (sockaddr *) &client, &len);
            char buffer[128];
            auto port = ntohs(client.sin_port);
            auto addr = inet_ntop(AF_INET, &client.sin_addr, buffer, sizeof(buffer));
            printf("Client fd[%d] from: %s:%d\n", new_fd, addr, port);
            connection[max_i + 1].fd = new_fd;
            //注册感兴趣的事件
            connection[max_i + 1].events = POLLIN;
            ++max_i;

            // 连接远程服务器
            auto remote_fd = socket(AF_INET, SOCK_STREAM, 0);
            auto flags_remote = fcntl(remote_fd, F_GETFL, 0);
            fcntl(remote_fd, flags_remote | O_NONBLOCK);
            remote.sin_port = htons(7788);
            remote.sin_family = AF_INET;
            inet_pton(AF_INET, "127.0.0.1", &remote.sin_addr);
            if (connect(remote_fd, (sockaddr *) &remote, sizeof(remote)) < 0) {
                //因为这时候不知道能不能发数据
                //所以把远程的fd加入poll监听
                //注册感兴趣的事件
                connection[max_i + 1].events = POLLOUT;
                connection[max_i + 1].fd = remote_fd;
                ++max_i;
            } else {
                //连接已经建立
                cout << "complete connect!" << endl;
                //注册写事件
                connection[max_i + 1].events = POLLOUT;
                connection[max_i + 1].fd = remote_fd;
                ++max_i;
            }
            // 创建新的连接对象
            TcPConnection *conn_local = new TcPConnection();
            TcPConnection *conn_remote = new TcPConnection();
            conn_local->fd = new_fd;
            conn_remote->fd = remote_fd;
            channel.push_back(conn_local);
            channel.push_back(conn_remote);

            // 设置pair
            auto *pair = new PairTcPConnection();
            pair->local = conn_local;
            pair->remote = conn_remote;
            tcp_pair.push_back(pair);

        } else {
            ready_i = max_i;
            for (int i = 1; i <= ready_i; ++i) {
                // 其它套接字可读，证明有新数据到来
                if (connection[i].revents == POLLIN) {
                    char buffer[1024]{0};
                    ssize_t read_n = read(connection[i].fd, buffer, sizeof(buffer) - 1);
                    if (read_n > 0) {
                        cout << buffer << endl;
                        // 数据存入buffer
                        TcPConnection *t = getTcPConnection(connection[i].fd);
                        t->read_buffer.insert(t->read_buffer.end(), buffer, buffer + read_n);
                        //这里要开启对远程服务器的写监听
                        for (auto p:tcp_pair) {
                            if (p->local->fd == connection[i].fd) {
                                for (auto &c:connection) {
                                    if (c.fd == p->remote->fd) {
                                        c.events = POLLOUT;
                                        break;
                                    }
                                }
                            }
                        }
                    } else if (read_n == 0) {
                        cout << "close fd: " << connection[i].fd << endl;
                        close(connection[i].fd);
                        connection[i].fd = -1;
                        --max_i;
                    } else {
                        cout << "read error!" << endl;
                        cout << strerror(errno) << endl;
                    }

                } else if (connection[i].revents & POLLOUT) {
                    cout << "poll out" << endl;
                    for (int j = 0; j < tcp_pair.size(); ++j) {
                        TcPConnection *p_conn_local = tcp_pair.at(j)->local;
                        TcPConnection *p_conn_remote = tcp_pair.at(j)->remote;
                        // 远程服务器可写
                        if (p_conn_remote->fd == connection[i].fd) {
                            //确保本地有数据
                            if (p_conn_local->read_buffer.size() > 0) {
                                printf("fd [%d] writble...\n", connection[i].fd);
                                //把本地服务器的数据转发给远程服务器
                                char send_buffer[2048]{0};
                                for (int k = 0; k < p_conn_local->read_buffer.size(); ++k) {
                                    send_buffer[k] = p_conn_local->read_buffer.at(k);
                                }
                                auto send_n = write(p_conn_remote->fd, send_buffer,
                                                    p_conn_local->read_buffer.size());
                                //关闭可写事件
                                if (send_n == p_conn_local->read_buffer.size()) {
                                    p_conn_local->read_buffer.clear();
                                    connection[i].events = 0;
                                }
                            } else {
                                //如果本地还没有数据,那么先关闭对remote fd的写监听
                                connection[i].events = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
