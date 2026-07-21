#include<bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>

constexpr int port = 8080;

int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("build socket");
        return 0;
    }
        
    int enable = 1;
    if(setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &enable,
        sizeof(enable)
    ) == -1){
        perror("set sock reuse addr");
        return 0;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // server_address.sin_addr.s_addr = 0;

    

}