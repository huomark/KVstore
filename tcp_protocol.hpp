char buffer[2048];
void send_all(int fd, std::string a){
    int gg = 0;
    while(gg<a.size()){
        int len = send(fd, a.data()+gg, a.size()-gg, 0);
        gg += len;
    }
}

std::vector<std::string> receive_all(int client_fd){
    int already_get = 0;
    std::vector<std::string> all_get;
    while(1){
        int gogo = recv(client_fd, buffer + already_get, sizeof(buffer) - 1 - already_get, 0);
        if(gogo <= 0){
            break;
        }
        already_get += gogo;
        while(already_get){
            buffer[already_get] = '\0';
            std::string ha = buffer;
            int fs = ha.find('\n');
            if(fs != -1){
                all_get.push_back(ha.substr(0, fs + 1));
                for(int i=fs+1;i<already_get;i++){
                    buffer[i-fs-1] = buffer[i];
                }
                already_get = already_get - fs - 1;
            }
            else break;
        }
        if(already_get == 0) break;
    }
    return all_get;
}