#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "tcp_protocol.hpp"

epoll_event ev, events[1005];
int main(){
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("build socket");
        return 0;
    }

    int epo = epoll_create(1); // just positive integer
    if (epo < 0) {
        perror("build epoll");
        return 0;
    }
    struct sockaddr_in ad{}, wtf{};
    ad.sin_family = AF_INET;
    ad.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &ad.sin_addr);
    bind(server_sock, (sockaddr*)&ad, sizeof(ad));
    listen(server_sock, 100);
    socklen_t wtf_len = (sizeof(wtf));

    ev.events = EPOLLIN;
    ev.data.fd = server_sock;
    epoll_ctl(epo, EPOLL_CTL_ADD, server_sock, &ev);

    while(1){
        int n = epoll_wait(epo, events, sizeof(events), -1);
        for(int i=0;i<n;i++){
            int ev_fd = events[i].data.fd;//data.fd
            if(ev_fd == server_sock){
                int cl = accept(server_sock, NULL, NULL);
                epoll_event wtf{};
                wtf.events = EPOLLIN;
                wtf.data.fd = cl;
                epoll_ctl(epo, EPOLL_CTL_ADD, cl, &wtf);
            }
            else{
                auto gain = receive_all(ev_fd);
                for(auto gg: gain) std::cout << gg;
            }
        }
    }

    // int client = accept(server_sock, (sockaddr*)&wtf, &wtf_len);   
    // std::cout<<client<<"\n";
}