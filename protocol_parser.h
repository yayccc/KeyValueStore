#pragma once

#include <string.h>

#include <iostream>
#include <string>

#include "connect_item.h"



const int MAX_TOKENS = 100;

class ProtocolParser{
public:
    //测试函数,解析并输出buffer中的字符串的tokens
    void TestParseTokens(int ep_fd,int conn_fd,char* rbuffer);

    //将buffer中的字符串解析成tokens,返回个数
    int ParseReadBuffer(char *buffer,char *tokens[]);

    //解析操作命令 返回tokens个数
    int ParseNetCommand(char *buffer,char *tokens[]);

    //解析“”中的字符串
    std::string ParseQuotedString(std::string &quoted_buffer,size_t &pos);

    //解析wal buffer
    int ParseWalBuffer(char* buffer,char *tokens[],bool is_crc32);


    char** GetTokensRBuffer(){
        return tokens_rbuffer_;
    }
private:
    int tokens_rbuffer_cnt_;
    char *tokens_rbuffer_[MAX_TOKENS];
    const char *tokens_rubffer_opeartion_[4] = {"SET","GET","MOD","DEL"};
};