#include <iostream>
#include "common.hpp"
#include <poll.h>
#include <string>
#include <cerrno>
#include <cstring>
/*
 * 学习poll的使用
 * */
using namespace std;

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
    pollfd client[1024]{0};
    client[0].fd = listen_fd;
    client[0].events = POLLIN;
    auto max_i = 0;
    auto ready_i = max_i;
    while (1) {
        auto ret = poll(client, max_i + 1, -1);

        if (client[0].revents == POLLIN) {
            unsigned int len = sizeof(cli);
            auto new_fd = accept(client[0].fd, (sockaddr *) &cli, &len);
            char buffer[128];
            auto port = ntohs(cli.sin_port);
            auto addr = inet_ntop(AF_INET, &cli.sin_addr, buffer, sizeof(buffer));
            printf("Client fd[%d] from: %s:%d\n", new_fd, addr, port);
            client[max_i + 1].fd = new_fd;
            //注册感兴趣的事件
            client[max_i + 1].events = POLLIN;
            ++max_i;
        } else {
            ready_i = max_i;
            for (int i = 1; i <= ready_i; ++i) {
                //可读
                if (client[i].events == POLLIN) {
                    char buffer[1024]{0};
                    ssize_t read_n = read(client[i].fd, buffer, sizeof(buffer) - 1);
                    if (read_n > 0) {
                        cout << buffer << endl;
                        //设置可读/写事件
                        client[i].events = POLLOUT | POLLIN;
                    } else if (read_n == 0) {
                        cout << "close fd: " << client[i].fd << endl;
                        close(client[i].fd);
                        client[i].fd = -1;
                        --max_i;
                    } else {
                        cout << "read error!" << endl;
                        cout << strerror(errno) << endl;
                    }

                } else if (client[i].revents == POLLOUT) {
                    printf("fd [%d] writble...\n", client[i].fd);
                    string msg = "send to fd ";
                    msg.append(to_string(client[i].fd));
                    auto send_n = write(client[i].fd, msg.c_str(), msg.size());
                    //关闭可写事件
                    if (send_n == msg.size()) {
                        client[i].events = POLLIN;
                    }

                }
            }
        }
    }
    return 0;
}
