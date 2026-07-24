#include<bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

constexpr int port = 8080;

char buf[2048];
char buffer[2048];

void send_all(int fd, std::string a){
    // std::serr<<
    a += '\n';
    int gg = 0;
    while(gg<a.size()){
        // std::cerr<<gg<<"\n";
        std::string b = a.substr(gg);
        int len = send(fd, b.data(), b.size(), 0);
        gg += len;
    }
}
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
    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if(connect(sockfd, (const sockaddr*)&server, sizeof(server)) == -1){
        perror("connect");
        return 0;
    }

    
    while(1){
        std::string a;
        std::getline(std::cin, a);
        send_all(sockfd, a);
        int gogo = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if(gogo == -1){
            perror("recv");
            return 0;
        }
        buffer[gogo] = '\0';
        std::cout<<"message is: "<<buffer<<"\n";
    }
    // server_address.sin_addr.s_addr = 0;

    

}