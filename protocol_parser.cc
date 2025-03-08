#include "protocol_parser.h"


//测试函数,解析并输出buffer中的字符串的tokens
void ProtocolParser::TestParseTokens(int ep_fd,int conn_fd,char* wbuffer){
    //创建一个连接对象
    ConnectItem connect_item;
    char **tokens;
    int tokens_rbuffer_cnt_ = ParseReadBuffer(wbuffer,tokens);
    
    std::cout << "tokens_cnt:" << tokens_rbuffer_cnt_<< std::endl;

    for(int i = 0;i<tokens_rbuffer_cnt_;i++){
        std::cout<<"token:"<<tokens[i]<<std::endl;

        connect_item.SetWbuffer(tokens[i]);

        connect_item.SendCb(ep_fd,conn_fd); 
    }
}



//解析buffer中的字符串，写入tokens[],返回token的个数
int ProtocolParser::ParseReadBuffer(char *buffer,char *tokens[]){

    int tokens_cnt = 0;
    char *token;
    tokens[tokens_cnt ++] = strtok(buffer," ");
    while((token = strtok(NULL," "))!= NULL){
        tokens[tokens_cnt ++] = token;
    }
    tokens[tokens_cnt] = NULL;

    return tokens_cnt;
}



//解析网络传输的命令<op> <key> <value>(允许使用“”包含有空格字符串)
int ProtocolParser::ParseNetCommand(char* buffer_line,char *tokens[])
{
    //data structure
    //<operation> <key> <value>

    std::string buffer;
    buffer = buffer_line;

    std::cout<<"buffer_line:"<< buffer<<std::endl;
    std::cout<<"buffer_line.size():"<<buffer.size()<<std::endl;

    //---解析 Operation---
    size_t pos = 0;
    //跳过空格
    while(pos<buffer.size() && buffer[pos] == ' ')pos++;
    //找到第一个空格
    int end = buffer.find(' ',pos);
    //没有找到空格
    if(end == std::string::npos){
        std::cout<<"error: no key and value"<<std::endl;
        return 0;
    }
    //找到了空格
    else{
        std::string operation = buffer.substr(pos,end - pos);
        tokens[0] = strdup(operation.c_str());
        pos = end;
    }
    
    //解析操作符，不匹配则返回0
    for(int i = 0;i<4;i++){
        if(strcmp(tokens[0],tokens_rubffer_opeartion_[i]) == 0){
            break;
        }
        else if(i == 3){
            std::cout<<"error: no operation"<<std::endl;
            return 0;
        }
    }
    std::cout<<"tokens[0]:"<<tokens[0]<<std::endl;
    

    //---解析key---
    //跳过空格
    while(pos<buffer.size() && buffer[pos] == ' ')pos++;
    //找到第一个空格
    end = buffer.find(' ',pos);
    //如果没有找到空格，则end指向buffer的末尾
    if(end == std::string::npos){
        end = buffer.size()+1;
    }
    //解析出key，存入tokens[1]
    std::string key = buffer.substr(pos,end-pos);
    tokens[1] = strdup(key.c_str());
    pos = end;

    std::cout<<"tokens[1]:"<<tokens[1]<<std::endl;  
    
    if(strcmp(tokens[0],"GET") == 0 || strcmp(tokens[0],"DEL") == 0){
        return 2;
    }

    //---解析value---

    std::string value;
    value = ParseQuotedString(buffer,pos);
    tokens[2] = strdup(value.c_str());

    // //检查冗余字符(末尾可能有换行符)
    // while(pos<buffer.size() && buffer[pos] == ' ')pos++;
    // if(pos != buffer.size()){
    //     std::cout<<"error: 命令末尾存在多余字符"<<std::endl;
    // }
    return 3;
}


//解析value(包含“”,支持转义字符\)
std::string ProtocolParser::ParseQuotedString(std::string &quoted_buffer, size_t &pos)
{      
    std::string value;
    char quote_char;
    while(pos < quoted_buffer.size()&&quoted_buffer[pos] == ' ')pos ++;
    
    if(quoted_buffer[pos] == '\"'){
        quote_char = '\"';
        pos++;        
    }
    else if(quoted_buffer[pos] == '\''){
        quote_char = '\'';
        pos++;
    }

    while(pos < quoted_buffer.size()){
        if(quoted_buffer[pos] == '\\'){
            pos++;
            if(pos > quoted_buffer.size()){
                std::cout<<"error: 无效的转义字符"<<std::endl;
                value += '\\';
                break;
            }
            value += quoted_buffer[pos];
            pos++;
        }
        else if(quoted_buffer[pos] == quote_char){
            pos++;
            return value;
        }
        else{
            value += quoted_buffer[pos];
            pos++;
        }
    }
    return value;
}

