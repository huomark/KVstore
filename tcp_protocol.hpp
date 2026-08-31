char buffer[20048];
std::pair<std::string, int> send_all(int fd, std::string a){
    int gg = 0;
    while(gg<a.size()){
        int len = send(fd, a.data()+gg, a.size()-gg, 0);
        if (len < 0){
            if(errno == EAGAIN){ // should use EPOLLOUT
                return {a.substr(gg), 2};
            }
            else // client disconnect.
                return {"", 0};
        }
        gg += len;
    }
    return {"", 1};
}

std::pair<std::vector<std::string>, bool> receive_all(int client_fd, std::string tmp){
    int already_get = 0;
    std::vector<std::string> all_get;
    bool ok = 1;
    while(1){
        int gogo = recv(client_fd, buffer + already_get, sizeof(buffer) - 1 - already_get, 0);
        if(gogo == 0){
            ok = 0;
            break;
        }
        if(gogo < 0){
            if(errno == EAGAIN){
                if(already_get){
                    buffer[already_get] = '\0';
                    std::string ha = buffer;
                    all_get.push_back(tmp+ha);
                    if(all_get.back().size()>1000){
                        return {{}, 0};
                    }
                    tmp="";
                }
            }
            else{
                perror("receive from client");
                ok = 0;
            }
            break;
        }
        already_get += gogo;
        // message too long
        while(already_get){
            buffer[already_get] = '\0';
            std::string ha = buffer;
            int fs = ha.find('\n');
            if(fs != -1){
                all_get.push_back(tmp + ha.substr(0, fs + 1));
                tmp = "";
                for(int i=fs+1;i<already_get;i++){
                    buffer[i-fs-1] = buffer[i];
                }
                already_get = already_get - fs - 1;
            }
            else break;
        }
        if(tmp.size() + already_get > 1000){
            return {{}, 0};
        }
    }
    return {all_get, ok};
}