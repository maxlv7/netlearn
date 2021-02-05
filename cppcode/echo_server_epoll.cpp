#include <iostream>
#include "common.hpp"
#include <sys/epoll.h>
#include <string>
#include <cerrno>
#include <cstring>
#include <vector>
/*
 * 学习epoll的使用
 * 使用epoll构建一个简单的echo服务端
 * */
using namespace std;
struct Client {
    vector<char> buffer;
    int fd;
};

int main() {
    auto listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server{};
    sockaddr_in cli{};

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
    //创建epoll实例
//    int epoll_fd = epoll_create(10);
    epoll_event event{};
    epoll_event ev_list[2048]{};
    Client client[2048]{};
    for (auto &c:client) {
        c.fd = -1;
    }
    auto client_total{0};
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    //修改epoll感兴趣的列表
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    for (;;) {
        int num = epoll_wait(epoll_fd, ev_list, 512, -1);
//        cout << "num: " << num << endl;
        if (num < 0) {
            continue;
        }
        for (int i = 0; i < num; ++i) {
            if (ev_list[i].data.fd == listen_fd) {
//                cout << "accept" << endl;
                socklen_t len = sizeof(cli);
                auto connfd = accept(listen_fd, (sockaddr *) &cli, &len);
                char buffer[128];
                auto port = ntohs(cli.sin_port);
                auto addr = inet_ntop(AF_INET, &cli.sin_addr, buffer, sizeof(buffer));
//                printf("Client fd[%d] from: %s:%d\n", connfd, addr, port);
                //为新建立连接的进程注册事件
                event.data.fd = connfd;
                event.events = EPOLLIN | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &event);
                //分配客户端
                ++client_total;
                for (int j = 0; j < client_total; ++j) {
                    //找一个空位
                    if (client[j].fd == -1) {
                        client[j].fd = connfd;
                        break;
                    }

                }

            } else if (ev_list[i].events & EPOLLIN) {
//                cout << "read" << endl;
                char buf[1024];
                int readn = read(ev_list[i].data.fd, buf, 1023);
                if (readn > 0) {
                    for (int j = 0; j < client_total; ++j) {
                        if (client[j].fd == ev_list[i].data.fd) {
                            auto &buf_v = client[j].buffer;
                            buf_v.insert(buf_v.end(), buf, buf + readn);
                            //再次设置为可读写
                            event.data.fd = ev_list[i].data.fd;
                            event.events = EPOLLIN | EPOLLOUT;
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev_list[i].data.fd, &event);
                            break;
                        }
                    }
                } else if (readn == 0) {
                    //客户端断开连接
//                    cout << "close fd: " << ev_list[i].data.fd << endl;
                    close(ev_list[i].data.fd);
                    //fd设置为-1
                    for (int j = 0; j < client_total; ++j) {
                        if (client[j].fd == ev_list[i].data.fd) {
                            client[j].fd = -1;
                            --client_total;
                            break;
                        }
                    }
                } else {
                    cout << "read error: " << strerror(errno) << endl;
                }

            } else if (ev_list[i].events & EPOLLOUT) {
                //有数据可写
//                cout << "write" << endl;
                for (int k = 0; k < client_total; ++k) {
                    if (client[k].fd == ev_list[i].data.fd) {
                        if (client[k].buffer.size() == 0) {
                            event.data.fd = client[k].fd;
                            event.events = EPOLLIN;
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client[k].fd, &event);
                            break;
                        }
                        auto writen = write(client[k].fd, client[k].buffer.data(), client[k].buffer.size());
                        if (writen == client[k].buffer.size()) {
                            client[k].buffer.clear();
                            event.data.fd = client[k].fd;
                            event.events = EPOLLIN;
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client[k].fd, &event);
                        }
                        break;
                    }
                }
            }
        }
    }
    return 0;
}
