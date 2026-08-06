char buffer[2048];
void send_all(int fd, std::string a){
    a += '\n';
    int gg = 0;
    while(gg<a.size()){
        int len = send(fd, a.data()+gg, a.size()-gg, 0);
        gg += len;
    }
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