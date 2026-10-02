#include<bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "tcp_protocol.hpp"
#include "hash_map.hpp"

constexpr int port = 8080;

const int EVENT_NUM = 12005;
struct Connection{
    std::string left_command, wait_reply;
    bool can_remove;
};
std::vector<Connection> connections(EVENT_NUM); 

void set_nonblock(int &fd){
    int ori_flag = fcntl(fd, F_GETFL, 0);
    ori_flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, ori_flag);
}


epoll_event events[EVENT_NUM];
constexpr int WAIT_LIMIT = 6500;
int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("build socket");
        return 0;
    }

    int epo = epoll_create(1); // just positive integer
    if (epo < 0) {
        perror("build epoll");
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

    {
        epoll_event server_ev{};
        server_ev.events = EPOLLIN;
        server_ev.data.fd = sockfd;
        if(epoll_ctl(epo, EPOLL_CTL_ADD, sockfd, &server_ev) == -1){
            perror("epoll add server");
            exit(0);
        }
    }
    
    auto kick_user = [&](int now_fd){
        connections[now_fd].wait_reply = "";
        connections[now_fd].left_command = "";
        connections[now_fd].can_remove = 0;
        epoll_ctl(epo, EPOLL_CTL_DEL, now_fd, nullptr);
        close(now_fd);
    };
    auto add_new_user = [&](int now_fd){
        epoll_event client_ep{};
        set_nonblock(now_fd);
        connections[now_fd].wait_reply = "";
        connections[now_fd].left_command = "";
        connections[now_fd].can_remove = 0;
        client_ep.events = EPOLLIN;
        client_ep.data.fd = now_fd;
        epoll_ctl(epo, EPOLL_CTL_ADD, now_fd, &client_ep);
    };

    auto epoll_mod = [&](int now_fd, int event){
        epoll_event client_ep{};
        client_ep.events = event;
        client_ep.data.fd = now_fd;
        epoll_ctl(epo, EPOLL_CTL_MOD, now_fd, &client_ep);
    };

    auto try_to_send_reply_to_client = [&](int now_fd){
        auto _ = send_all(now_fd, connections[now_fd].wait_reply);
            int on = _.second;
        if(!on) {
            kick_user(now_fd);
        }
        else if(on == 2){
            connections[now_fd].wait_reply = _.first;
            if(connections[now_fd].wait_reply.size() > WAIT_LIMIT){
                kick_user(now_fd);
                return;
            }
            epoll_mod(now_fd, EPOLLIN | EPOLLOUT);
        }
        else{
            if(connections[now_fd].can_remove){
                kick_user(now_fd);
                return;
            }
            epoll_mod(now_fd, EPOLLIN);
            connections[now_fd].wait_reply = "";
        }
    };

    while(1){
        int n = epoll_wait(epo, events, EVENT_NUM, -1);
        for(int i = 0; i < n; i++){
            int now_fd = events[i].data.fd;
            if(now_fd == sockfd){
                int client_sock = accept(sockfd, NULL, NULL);
                if(client_sock == -1) continue;
                add_new_user(client_sock);
            }
            else{
                bool online = 1;
                if(events[i].events & EPOLLIN){
                    std::string now = connections[now_fd].left_command;
                    auto [receive, online] = receive_all(now_fd, now);
                    if(online == 0){
                        connections[now_fd].can_remove = 1;
                    }
                    if(receive.size() && receive.back()[receive.back().size()-1] != '\n'){
                        connections[now_fd].left_command = receive.back();;
                        receive.pop_back();
                    }
                    else connections[now_fd].left_command = "";
                    for(auto all_com: receive){
                        auto pC = parseCommand(all_com);
                        std::string rep = "fail\n";
                        if(check_valid_command(pC)) {
                            rep = todo(pC);
                        }
                        connections[now_fd].wait_reply += rep;
                    }
                    try_to_send_reply_to_client(now_fd);
                }

                if(events[i].events & EPOLLOUT){
                    try_to_send_reply_to_client(now_fd);
                }
            }
        }
    }
    // server_address.sin_addr.s_addr = 0;

}