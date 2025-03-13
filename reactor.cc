#include "reactor.h"

Reactor::Reactor()
{
    //epoll初始化
    ep_fd = epoll_create(1);
    //初始化监听套接字
    listen_fd = sock.Init(ep_fd);
}



void Reactor::Run()
{
    //恢复数据
    logfile.RestoreFromSingleLog(map,ENABLE_CRC32);

    ThreadPool pool(4);
    //开始监听
    while(1){
        ep_cnt = check_error("epoll_wait",epoll_wait(ep_fd,events,1024,-1));
        for(int i = 0;i < ep_cnt;i ++){

            int conn_fd = events[i].data.fd;

            //EPOLLIN事件
            if(events[i].events == EPOLLIN){
                std::cout<<"EPOLLIN"<<std::endl;
                pool.Enqueue(&Reactor::EpollIn,this,ep_fd,conn_fd);

            }
            //EPOLLOUT事件
            else if(events[i].events == EPOLLOUT){
                std::cout<<"EPOLLOUT"<<std::endl;
                pool.Enqueue(&Reactor::EpollOut,this,ep_fd,conn_fd);
            }
        }
    }

    close(listen_fd);
    close(ep_fd);
}



//EPOLLOUT事件,发送数据
void Reactor::EpollOut(int ep_fd, int conn_fd)
{
    //发送数据
    connlist[conn_fd].SendCb(ep_fd,conn_fd);
    //设置为可读事件
    connlist[conn_fd].SetReadyRead(ep_fd,conn_fd);
    return ;
}



//EPOLLIN事件,接受连接和接受数据，解析数据
void Reactor::EpollIn(int ep_fd, int conn_fd)
{   
    //accept
    std::cout<<"conn_fd:"<<conn_fd<<std::endl;
    if(conn_fd == listen_fd){
        std::cout<<"accept"<<std::endl; 
        connlist[conn_fd].AcceptCb(ep_fd,listen_fd,client_addr);
    }
    //recv
    else{
        //接收数据
        int ret = connlist[conn_fd].RecvCb(ep_fd,conn_fd);
        //断开连接
        if(ret == 0){
            epoll_ctl(ep_fd,EPOLL_CTL_DEL,conn_fd,NULL);
            close(conn_fd);
            return ;
        }

        //tokens
        char *tokens[MAX_TOKENS];

        //拿到并输出读缓冲区数据 
        char *rbuffer; 
        connlist[conn_fd].GetRbuffer(rbuffer);
        std::cout<<"rbuffer:"<<rbuffer<<std::endl;

        //解析
        int tokens_cnt = parser.ParseNetCommand(rbuffer,tokens);

        //使用map引擎
        int operate = map.GetOperate(tokens[0]);

        //如果操作码为-1，则操作错误
        if(operate == -1){
            std::cout<<"operate error"<<std::endl;
            connlist[conn_fd].SetWbuffer("OPERATE ERROR");
            connlist[conn_fd].SetReadyWrite(ep_fd,conn_fd);
            return ;
        }
        //执行操作
        else
        {
            switch(operate){
                //SET操作
                case 0:{
                    logfile.WriteToSingleLog(tokens,tokens_cnt,ENABLE_CRC32);
                    std::cout<<"set"<<std::endl;
                    map.Set(tokens);
                    connlist[conn_fd].SetWbuffer("SET COMPLETE");
                    break;                               
                }
                //GET操作
                case 1:{
                    std::cout<<"get"<<std::endl;
                    std::string value;
                    value = map.Get(tokens);
                    connlist[conn_fd].SetWbuffer((const char*)value.c_str());
                    break;                                
                }
                //MOD操作
                case 2:{
                    logfile.WriteToSingleLog(tokens,tokens_cnt,ENABLE_CRC32);
                    std::cout<<"mod"<<std::endl;
                    map.Mod(tokens);
                    connlist[conn_fd].SetWbuffer("MOD COMPLETE");
                    break;                                
                }
                //DEL操作
                case 3:{
                    logfile.WriteToSingleLog(tokens,tokens_cnt,ENABLE_CRC32);
                    std::cout<<"del"<<std::endl;
                    map.Del(tokens);
                    connlist[conn_fd].SetWbuffer("DEL COMPLETE");
                    break;                                
                }
                //操作错误
                default:{
                    std::cout<<"operate error"<<std::endl;
                    connlist[conn_fd].SetWbuffer("OPERATE ERROR");
                    break;                                
                }
            }

            //设置为可写事件
            connlist[conn_fd].SetReadyWrite(ep_fd,conn_fd);
        }

        //释放tokens(在解析器中分配的内存)
        for(int i = 0;i<tokens_cnt;i++){
            free(tokens[i]);
        }
    }
    return ;
}
