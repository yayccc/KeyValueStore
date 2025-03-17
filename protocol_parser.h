#pragma once

#include <string.h>

#include <iostream>
#include <string>

#include "config.h"

class MapEngine;
class WriteAheadLog;


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

    void SetOptFlag(bool flag){opt_flag = flag;}
    void SetKeyFlag(bool flag){key_flag = flag;}
    void SetValueFlag(bool flag){value_flag = flag;}
    bool GetOptFlag(){return opt_flag;}
    bool GetKeyFlag(){return key_flag;}
    bool GetValueFlag(){return value_flag;}

private:
    bool opt_flag = false;
    bool key_flag = false;
    bool value_flag = false;
    const char *tokens_rubffer_opeartion_[4] = {"SET","GET","MOD","DEL"};
    
};



//RESP协议解析器
class RespParser : public ProtocolParser{
public:
    //解析RESP协议
    std::string ParseRespCommand(char *buffer,char *tokens[],MapEngine &map);

    //解析RESP协议中的数字
    int ParseRespNum(std::string &buffer,int &index,std::string &ret);

    //跳过\r\n，index指向下一个字符，如果\r\n不存在，index不变
    int SkipCRLF(std::string &buffer,int &index,std::string &ret);
    
    //解析操作符
    int ParseOperation(std::string &buffer,int &index,char *tokens[],std::string &ret);

    //解析key
    int ParseKey(std::string &buffer,int &index,char *tokens[],std::string &ret);

    //解析value
    int ParseValue(std::string &buffer,int &index,char *tokens[],std::string &ret);

    //错误处理
    void ErrorHandle(std::string &ret);

private:
    
};