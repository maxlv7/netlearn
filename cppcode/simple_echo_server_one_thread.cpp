#include <iostream>
#include <unistd.h>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main() {

    struct sockaddr_in sin{};
    sin.sin_port = htons(7777);
    sin.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(sock, (sockaddr *) &sin, sizeof(sin))) {
        cout << "bind error" << endl;
    }

    listen(sock, 10);
    struct sockaddr_in client{};
    socklen_t addr_len = sizeof(client);
    int time =100;
    while (time--) {
        int accsock = accept(sock, (sockaddr *) &client, &addr_len);
        cout << "new sock:" << accsock;
        char ip[128];
        cout << " from " << inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(client)) << ":" << client.sin_port << endl;
        char buff[1024];
        while (true) {
            int size = read(accsock, buff, sizeof(buff) - 1);
            if (size <= 0) {
                cout << "close" << accsock << endl;
                close(accsock);
                break;
            } else if (size > 0) {
//                cout << buff << endl;
                write(accsock, buff, size);
            }
        }
    }
    return 0;
}
