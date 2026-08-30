#include<bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "tcp_protocol.hpp"

constexpr int port = 8080;

void set_nonblock(int &fd){
    int ori_flag = fcntl(fd, F_GETFL, 0);
    ori_flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, ori_flag);
}

bool check_valid_command(std::vector<std::string> now){
    if(now[0] == "SET"){
        if(now.size()==3) return 1;
    }
    else if(now[0] == "GET"){
        if(now.size()==2) return 1;
    }
    else if(now[0] == "DEL")
        if(now.size()==2) return 1;
    return 0;
}

std::unordered_map<std::string, std::string>mp;

std::string todo(std::vector<std::string> now){
    std::string reply = "";
    if(now[0] == "SET"){
        mp[now[1]] = now[2];
        reply = "SET successfully\n";
    }
    else if(now[0] == "GET"){
        if(mp.count(now[1])){
            reply = mp[now[1]];
            reply += '\n';
        }    
        else
            reply = "* KEY doesn't exist\n";
    }
    else if(now[0] == "DEL"){
        if(mp.count(now[1])){
            mp.erase(now[1]);
            reply = "DEL successfully\n";
        }    
        else
            reply = "* KEY doesn't exist\n";
    }
    return reply;
    
}

std::vector<std::string> parseCommand(std::string raw){
    std::vector<std::string>xd;
    std::string now;
    for(auto tt: raw){
        if(tt==' '){
            xd.push_back(now);
            now="";
        }
        else if(tt=='\n') {
            xd.push_back(now);
            break;
        }
        else{
            now+=tt;
        }
    }

    return xd;
}

const int EVENT_NUM = 12005;
epoll_event events[EVENT_NUM];
std::string left_command[EVENT_NUM];
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
    
    while(1){
        int n = epoll_wait(epo, events, EVENT_NUM, -1);
        for(int i = 0; i < n; i++){
            int now_fd = events[i].data.fd;
            if(now_fd == sockfd){
                int client_sock = accept(sockfd, NULL, NULL);
                if(client_sock == -1) continue;
                set_nonblock(client_sock);
                epoll_event client_ep{};
                left_command[client_sock] = "";
                client_ep.events = EPOLLIN;
                client_ep.data.fd = client_sock;
                epoll_ctl(epo, EPOLL_CTL_ADD, client_sock, &client_ep);
            }
            else{
                std::string now = left_command[now_fd];
                auto pp = receive_all(now_fd, now);
                auto gogo = pp.first;
                bool online = pp.second;
                if(gogo.size() && gogo.back()[gogo.back().size()-1] != '\n'){
                    auto bk = gogo.back();
                    left_command[now_fd] = bk;
                    gogo.pop_back();
                }
                else left_command[now_fd] = "";
                for(auto all_com: gogo){
                    auto pC = parseCommand(all_com);
                    bool ok = check_valid_command(pC);
                    std::string rep = "fail\n";
                    if(ok) {
                        rep = todo(pC);
                    }
                    bool on = send_all(now_fd, rep);
                    if(!on) online = 0;
                }
                if(online == 0){
                    left_command[now_fd] = "";
                    epoll_ctl(epo, EPOLL_CTL_DEL, now_fd, nullptr);
                    close(now_fd);
                    continue;
                }
            }
        }
    }
    // server_address.sin_addr.s_addr = 0;


}