#pragma once
#include "main.h"

class Reactor{

public:
    //构造函数
    Reactor();

    //运行
    void Run();
    
    //EPOLLOUT事件,发送数据
    void EpollOut(int ep_fd,int conn_fd);

    //EPOLLIN事件,接收数据
    void EpollIn(int ep_fd, int conn_fd);

private:
    //监听套接字初始化
    SockInit sock;
    //连接队列
    ConnectItem connlist[65536];
    //map引擎
    MapEngine map;   
    //解析器
    ProtocolParser parser;   
    //单日志文件
    WriteAheadLog logfile;

    //epoll事件
    int ep_fd ,ep_cnt;
    epoll_event events[MAX_EVENTS];
    epoll_event event;

    //监听套接字
    int listen_fd;
    //客户端地址
    struct sockaddr_in client_addr;
  
    std::condition_variable condition_[65536];
    std::mutex mutex_;  
};