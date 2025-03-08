


#include "main.h"

ConnectItem connlist[1024];
MapEngine map;   
    //创建解析器对象
    ProtocolParser parser;   

int main(int argc,char* argv[]){

    WriteAheadLog logfile;
    //logfile.OpenSingleLogFile(std::ios::in | std::ios::out | std::ios::app |std::ios::binary);
    logfile.RestoreFromSingleLog(map,ENABLE_CRC32);


    //epoll初始化
    int ep_fd ,ep_cnt;
    struct sockaddr_in client_addr;
    
    //epoll事件数组
    epoll_event events[MAX_EVENTS];
    epoll_event event;
    ep_fd = epoll_create(1);

    //监听套接字初始化
    SockInit sock;
    int listen_fd = sock.Init(ep_fd);

    //tokens
    char *tokens[MAX_TOKENS];


    //读缓冲区指针
    char *wbuffer;
    //写缓冲区指针
    char *rbuffer;

    //开始监听
    while(1){
        ep_cnt = check_error("epoll_wait",epoll_wait(ep_fd,events,1024,-1));

        for(int i = 0;i < ep_cnt;i ++){
            int conn_fd = events[i].data.fd;
            //EPOLLIN事件
            if(events[i].events == EPOLLIN){
                //accept
                if(conn_fd == listen_fd){
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
                        continue;
                    }
                    
                    //拿到并输出读缓冲区数据
                    connlist[conn_fd].GetRbuffer(rbuffer);
                    std::cout<<"rbuffer:"<<rbuffer<<std::endl;

                    //解析
                    int tokens_cnt = parser.ParseNetCommand(rbuffer,tokens);
                    for(int i= 0;i<3;i++){
                        std::cout<<"tokens:"<<tokens[i]<<std::endl;
                    }                    

                    //使用map引擎
                    int operate = map.GetOperate(tokens[0]);

                    //如果操作码为-1，则操作错误
                    if(operate == -1){
                        std::cout<<"operate error"<<std::endl;
                        connlist[conn_fd].SetWbuffer("OPERATE ERROR");
                        break;
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
                    }
                    
                    // wbuffer = rbuffer;
                    //释放tokens
                    for(int i = 0;i<tokens_cnt;i++){
                        free(tokens[i]);
                    }
                }
            }
            //EPOLLOUT事件
            else if(events[i].events == EPOLLOUT){
                //测试解析
                //parser.TestParseTokens(ep_fd,conn_fd,wbuffer);

                connlist[conn_fd].SendCb(ep_fd,conn_fd);
            }
        }
    }
    
    close(listen_fd);
    close(ep_fd);
}


