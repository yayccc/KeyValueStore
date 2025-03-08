#pragma once

#include <string.h>

#include <iostream>
#include <string>

#include "connect_item.h"


const int MAX_TOKENS = 10;

class ProtocolParser{

public:
    //测试函数,解析并输出buffer中的字符串的tokens
    void TestParseTokens(int ep_fd,int conn_fd,char* rbuffer);

    //将buffer中的字符串解析成tokens,返回个数
    int ParseReadBuffer(char *buffer,char *tokens[]);

    //解析网络rbuffer操作命令 返回tokens个数
    int ParseNetCommand(char *buffer,char *tokens[]);

    //解析“”中的字符串
    std::string ParseQuotedString(std::string &quoted_buffer,size_t &pos);

private:

    const char *tokens_rubffer_opeartion_[4] = {"SET","GET","MOD","DEL"};
    
};