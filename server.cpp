#include<bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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
    
    struct sockaddr_in q{};
    q.sin_family = AF_INET;
    q.sin_port = htons(8080);
    q.sin_addr.s_addr = inet_addr("0.0.0.0");



    if(bind(sockfd, (struct sockaddr*)&q, sizeof(q)) == -1){
        perror("bind");
        return 0;
    }
    if(listen(sockfd, 100) == -1){
        perror("listen");
        return 0;
    }
    
    char buffer[2048];
    int client_fd = accept(sockfd, NULL, NULL);
    if(client_fd==-1){
        perror("accept");
        return 0;
    }
    while(1){
        int gogo = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if(gogo <= 0){
            break;
        }
        buffer[gogo] = '\0';
        std::cout<<"Recv: "<<buffer<<"\n";
        int se = send(client_fd, buffer, gogo + 1, 0);
    }
    // server_address.sin_addr.s_addr = 0;


}