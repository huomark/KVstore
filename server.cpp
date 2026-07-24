#include<bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

constexpr int port = 8080;

char buffer[2048];

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

std::vector<std::string> receive_all(int client_fd){
    int now = 0;
    std::vector<std::string> all_get;
    while(1){
        int gogo = recv(client_fd, buffer + now, sizeof(buffer) - 1 - now, 0);
        if(gogo <= 0){
            break;
        }
        now += gogo;
        while(now){
            buffer[now] = '\0';
            std::string ha = buffer;
            int fs = ha.find('\n');
            if(fs != -1){
                all_get.push_back(ha.substr(0, fs + 1));
                for(int i=fs+1;i<now;i++){
                    buffer[i-fs-1] = buffer[i];
                }
                now = now - fs - 1;
            }
            else break;
        }
        if(now == 0) break;
    }
    return all_get;
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
    
    int client_fd = accept(sockfd, NULL, NULL);
    if(client_fd==-1){
        perror("accept");
        return 0;
    }
    while(1){
        auto gogo = receive_all(client_fd);
        if(gogo.size() == 0){
            break;
        }
        for(auto all_com: gogo){
            auto pC = parseCommand(all_com);
            bool ok = check_valid_command(pC);
            std::string rep = "fail\n";
            if(ok) {
                rep = todo(pC);
            }
            int se = send(client_fd, rep.data(), rep.size(), 0);
        }
        // std::cout<<"Recv: "<<buffer<<"\n";
    }
    // server_address.sin_addr.s_addr = 0;


}