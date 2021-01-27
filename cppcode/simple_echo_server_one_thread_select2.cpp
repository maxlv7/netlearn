#include <iostream>
#include <cerrno>
#include <cstring>

#include <unistd.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

using namespace std;


int main() {
    struct sockaddr_in sin{};
    sin.sin_port = htons(7788);
    sin.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    cout << "open fd: " << sockfd << endl;
    if (bind(sockfd, (sockaddr *) &sin, sizeof(sin))) {
        cout << "bind error" << endl;
    }
//    int opt = 1;
//    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    listen(sockfd, 10);
    struct sockaddr_in client{};
    socklen_t addr_len = sizeof(client);
    int fd[1024] = {0};
    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_ex;
    fd_set tmp_read_fd;
    FD_ZERO(&fd_read);
    FD_ZERO(&tmp_read_fd);
    int max_sock = sockfd;
    int i;
    FD_SET(sockfd, &fd_read);
    while (true) {
        tmp_read_fd = fd_read;
        int ret = select(max_sock + 1, &tmp_read_fd, 0, 0, 0);
        if (ret < 0) {
            cout << strerror(errno) << endl;
            break;
        } else if (ret == 0) {
            cout << "time out" << endl;
        }
        // 有文件描述符可读
        if (FD_ISSET(sockfd, &tmp_read_fd)) {
            // 选择合适的I
            for (i = 0; i < 1024; ++i) {
                if (fd[i] == 0) {
                    break;
                }
            }
            fd[i] = accept(sockfd, (sockaddr *) &client, &addr_len);
            if (fd[i] == -1) {
                cerr << "accept error" << strerror(errno) << endl;
            }
            FD_SET(fd[i], &fd_read);
            cout << "new sockfd:" << fd[i] << endl;
            if (max_sock < fd[i]) {
                max_sock = fd[i];
            }
        } else {
            for (i = 0; i < 1024; ++i) {
                if (FD_ISSET(fd[i], &tmp_read_fd)) {
                    char buf[1024]{0};
                    ssize_t s = read(fd[i], buf, sizeof(buf) - 1);
                    if (s == -1) {
                        cerr << "recv error" << endl;
                    } else if (s == 0) {
                        cout << "close fd: " << fd[i] << endl;
                        close(fd[i]); //关TCP
                        FD_CLR(fd[i], &fd_read); //从集合中清掉fd
                        fd[i] = 0;
                    } else {
                        //模拟请求其它服务器
                        struct sockaddr_in request{};
                        request.sin_port = htons(7777);
                        request.sin_family = AF_INET;
                        inet_pton(AF_INET, "127.0.0.1", &request.sin_addr);
                        int request_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        cout << "request_fd: " << request_fd << endl;
                        int con_state = connect(request_fd, (sockaddr *) &request, sizeof(request));
                        if (con_state < 0) {
                            cerr << strerror(errno) << endl;
                        } else {
                            // 假如从远程服务器获取数据是阻塞的
                            char buff_remote[1024]{0};
                            write(request_fd, "hello from select 2", 19);
                            ssize_t remote = read(request_fd, buff_remote, 1024);
                            if (remote>0){
                                printf("remote read:%s",buff_remote);
                            }else if(remote<=0){
                                cout << "close remote" << endl;
                                close(request_fd);
                            }
                        }
                        printf("read [%zd] byte from fd[%d] content[%s]\n", s, fd[i], buf);
                    }
                    break;
                }
            }


        }
    }

//    char ip[128];
//    cout << " from " << inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(client)) << ":" << client.sin_port << endl;
    return 0;
}
