#include "connect_item.h"



//初始化监听套接字
int SockInit::Init(int ep_fd){

    //创建监听套接字
    listen_fd_ = check_error("socket",socket(PF_INET,SOCK_STREAM,0));
    std::cout<<"listen_fd:"<<listen_fd_<<std::endl;    
    //设置server地址
    serv_addr_.sin_family = AF_INET;
    serv_addr_.sin_port = htons(CONNECT_PORT);
    serv_addr_.sin_addr.s_addr = htonl(INADDR_ANY);

    //绑定
    check_error("bind",bind(listen_fd_,(struct sockaddr*)&serv_addr_,sizeof(serv_addr_)));
    //监听
    check_error("listen",listen(listen_fd_,1000));
    std::cout<<"listen && bind"<<std::endl;

    //设置事件为EPOLLIN，添加到epoll事件表
    event_.events = EPOLLIN | EPOLLET;
    event_.data.fd = listen_fd_;
    check_error("epoll_ctl",epoll_ctl(ep_fd,EPOLL_CTL_ADD,listen_fd_,&event_));\

    std::cout<<"epoll_ctl"<<std::endl;

    return listen_fd_;
}


//连接客户端
void ConnectItem::AcceptCb(int ep_fd,int listen_fd, struct sockaddr_in &client_addr){

    std::cout<<"accept"<<std::endl;
    //接受连接
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = check_error("accept", accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len));

    std::cout<<"client_fd:"<<client_fd<<std::endl;
    //设置事件为EPOLLIN，添加到epoll事件表
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;

    epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &event);
    std::cout<<"epoll_ctl"<<std::endl;
    //打印客户端连接信息
    printf("connect:%d\n", client_fd);
}


//接收数据,返回rlen_,<0表示无输入,0表示断开连接,>0表示接收到的字节数
int ConnectItem::RecvCb(int ep_fd,int clnt_fd){
    //invalid file descriptor
    if(clnt_fd < 0){
        std::cout<<"Invalid file descriptor:"<<clnt_fd<<std::endl;
        exit(-1);
    }

    rlen_ = recv(clnt_fd,&rbuffer_,sizeof(rbuffer_),MSG_DONTWAIT);

    //recv error
    if(rlen_ < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return rlen_;
        }
        else{
            perror("recv_cb");
            exit(-1);
        }
    }
    //disconnect
    else if(rlen_ == 0){
        epoll_ctl(ep_fd,EPOLL_CTL_DEL,clnt_fd,NULL);
        printf("close:%d\n",clnt_fd);
        close(clnt_fd);
        return rlen_;
    }
    //recv success,set ready to write
    else{
        rbuffer_[rlen_] = '\0';
        
        return rlen_;
    }
}



//发送数据,返回wlen_,<0表示无可写,0表示断开连接,>0表示发送的字节数
int ConnectItem::SendCb(int ep_fd,int clnt_fd){
    //invalid file descriptor
    if (clnt_fd < 0) {
        std::cerr << "Invalid file descriptor: " << clnt_fd << std::endl;
        exit(-1);
    }

    wlen_ = send(clnt_fd,&wbuffer_,wlen_,MSG_DONTWAIT);

    //send error
    if(wlen_ < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return wlen_;          
        }
        else{
            perror("send_cb");
            exit(-1);
        }
    }
    //disconnect
    else if(wlen_ == 0){
        epoll_ctl(ep_fd,EPOLL_CTL_DEL,clnt_fd,NULL);
        printf("close2:%d",clnt_fd);
        sleep(3);
        close(clnt_fd);
        return wlen_;
    }
    //send success
    else{
        //clear buffer
        memset(&wbuffer_,0,sizeof(wbuffer_));
        int wlen = wlen_;
        wlen_ = 0;
        return wlen;
    }
}

