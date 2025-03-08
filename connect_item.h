#pragma once

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<errno.h>
#include<fcntl.h>

#include<iostream>
#include<string>

//连接端口
const int CONNECT_PORT = 3030;
//最大事件数
const int MAX_EVENTS = 1024;

const int MAX_BUFFER = 1024;
const int BUFFER_SIZE = 1024;

//检查错误码(-1),无错误返回res值
inline int check_error(const char* msg,int res){
    if(res ==-1){
        perror(msg);
        exit(-1);
    }
    return res;
}

//套接字初始化
class SockInit{
public:
    //初始化监听套接字
    int Init(int ep_fd);

private:
    //监听套接字
    int listen_fd_;
    //服务器地址
    struct sockaddr_in serv_addr_;
    //epoll事件
    struct epoll_event event_;
};



//客户端与服务器的连接队列以及回调函数
class ConnectItem{
public:
    ConnectItem(){
        rlen_ = 0;
        wlen_ = 0;
    }
    //连接客户端
    void AcceptCb(int ep_fd,int listen_fd, struct sockaddr_in &client_addr);
    //接收数据
    int RecvCb(int ep_fd,int clnt_fd);
    //发送数据
    int SendCb(int ep_fd,int clnt_fd);

    //拿到写缓冲区数据
    void GetWbuffer(char* &buffer){
        buffer = wbuffer_;
    }
    //设置写缓冲区数据
    void SetWbuffer(const char* buffer){
        if(buffer == NULL){
            return;
        }
        strncpy(wbuffer_,buffer,sizeof(wbuffer_)-1);
        wbuffer_[BUFFER_SIZE - 1] = '\0';
        wlen_ = strlen(wbuffer_);
    }

    //拿到读缓冲区数据
    void GetRbuffer(char* &buffer){
        buffer = rbuffer_;
    }
    //设置读缓冲区数据
    void SetRbuffer(char* buffer){
        if(buffer == NULL){
            return;
        }
        strncpy(rbuffer_,buffer,sizeof(rbuffer_)-1);
        rbuffer_[BUFFER_SIZE - 1] = '\0';
        rlen_ = strlen(rbuffer_);
    }

private:
    //连接套接字
    int connfd_;

    //读缓冲区
    char rbuffer_[BUFFER_SIZE];
    
    //读缓冲区长度
    int rlen_;

    //写缓冲区
    char wbuffer_[BUFFER_SIZE];

    //写缓冲区长度
    int wlen_;

};
    

