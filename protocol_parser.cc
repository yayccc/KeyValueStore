
#include "protocol_parser.h"
#include "map_engine.h"
#include "connect_item.h"
#include "write_ahead_log.h"

WriteAheadLog parser_logfile;


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


//bug 1.如果有些命令的len与实际不符，会导致后续都不能解析
//bug 2.如果*后面的数字不止一位，会导致后续都不能解析，已解决
//暂时只能解析符合协议的命令，不符合的命令会解析失败

//解析RESP协议
std::string RespParser::ParseRespCommand(char *buffer, char *tokens[],MapEngine &map)
{   
    //字符串指针
    int i = 0;
    int tokens_cnt = 0;
    bool opt_flag = false;
    bool key_flag = false;
    bool value_flag = false;

    int opt_cnt =0;
    std::string ret_cnt = "*";
    std::string ret ;
    std::string resp_buffer;
    resp_buffer = buffer;

    //检查RESP协议
    if(resp_buffer[0] == '*'){
        i++; 
    }
    else{
        std::cout<<"-"<<std::endl;
        ret += "-ERR\r\n";
        return ret;
    }

    //解析*数组长度
    ParseRespNum(resp_buffer,i,ret);

    while(i < resp_buffer.length()-1){
        //跳过\r\n
        SkipCRLF(resp_buffer,i,ret);  

        //解析操作符
        if(ParseOperation(resp_buffer,i,tokens,ret) == 1 && !opt_flag){
            opt_flag = true;
        }

        //跳过\r\n
        SkipCRLF(resp_buffer,i,ret);

        //解析key
        if(ParseKey(resp_buffer,i,tokens,ret) == 0 && opt_flag && !key_flag){
            key_flag = true;
        }

        SkipCRLF(resp_buffer,i,ret);

        //解析value,GET和DEL操作不需要value
        if(opt_flag && key_flag && !value_flag){
            if(strcmp(tokens[0],"GET") !=0 && strcmp(tokens[0],"DEL") != 0){
                ParseValue(resp_buffer,i,tokens,ret);
                value_flag = true;
            }  
            else{
                value_flag = true;
            }          
        }

        SkipCRLF(resp_buffer,i,ret);

        //解析完成,执行操作,并释放tokens
        if(opt_flag && key_flag && value_flag){

            int operate = map.GetOperate(tokens[0]);
            std::cout<<"parser operate:"<<operate<<std::endl;
            switch (operate){
                case 0:{
                    parser_logfile.WriteToSingleLog(tokens,3,ENABLE_CRC32);
                    map.Set(tokens);
                    tokens_cnt = 3;
                    std::cout<<"SET"<<" "<<tokens[1]<<" "<<tokens[2]<<std::endl;
                    ret += "+OK\r\n";
                    break;
                }
                case 1:{
                    std::string get_value = map.Get(tokens);
                    int len = get_value.length();
                    std::string len_str = std::to_string(len);
                    tokens_cnt = 2;
                    std::cout<<"GET"<<" "<<tokens[1]<<std::endl;
                    ret += "$";
                    ret += len_str;
                    ret += "\r\n";
                    ret += get_value;
                    ret += "\r\n";
                    break;
                }
                case 2:{
                    parser_logfile.WriteToSingleLog(tokens,3,ENABLE_CRC32);
                    map.Mod(tokens);
                    tokens_cnt = 3;
                    std::cout<<"MOD"<<" "<<tokens[1]<<" "<<tokens[2]<<std::endl;
                    ret += "+OK\r\n";
                    break;
                }
                case 3:{
                    parser_logfile.WriteToSingleLog(tokens,2,ENABLE_CRC32);
                    map.Del(tokens);
                    tokens_cnt = 2;
                    std::cout<<"DEL"<<" "<<tokens[1]<<std::endl;
                    ret += "+OK\r\n";
                    break;
                }
            }

            for(int free_cnt = 0;free_cnt<tokens_cnt;free_cnt++){
                free(tokens[free_cnt]);
            }            
            tokens_cnt = 0;
            opt_flag = false;
            key_flag = false;
            value_flag = false;
            opt_cnt ++;
        }
    }
    
    std::string opt_str = std::to_string(opt_cnt);
    ret_cnt += opt_str;
    ret_cnt += "\r\n";
    return ret_cnt + ret;
}



//解析RESP协议中的数字
int RespParser::ParseRespNum(std::string &buffer, int &index,std::string &ret)
{   
    int num = 0;
    while(buffer[index] != '\r'){
        num = num * 10 + buffer[index] - '0';
        index++;
    }
    index+=2;
    return num;
}



//跳过\r\n，index指向下一个字符，如果\r\n不存在，index不变
int RespParser::SkipCRLF(std::string &buffer, int &index,std::string &ret)
{
    while(buffer[index] == '\r' && buffer[index+1] == '\n'){ 
        index+=2;
    }
    if(buffer[index] == '\r')index++;
    if(buffer[index] == '\n') index++;
    
    return 1;
}



//解析操作符
int RespParser::ParseOperation(std::string &buffer, int &index, char *tokens[],std::string &ret)
{          
    std::cout<<"index:"<<index<<std::endl;

    //找到第一个$
    index = buffer.find('$',index);
    if(index == std::string::npos){
        ret += "-ERR FIND NO CMD\r\n";
        return -1;
    }
    index++;

    //解析操作符长度
    int len = ParseRespNum(buffer,index,ret);

    //跳过\r\n  
    SkipCRLF(buffer,index,ret);

    if(len == 3){
        tokens[0] = strdup(buffer.substr(index,len).c_str());
        //解析操作符，不匹配则返回-1
        if(!strcmp(tokens[0],"SET") || !strcmp(tokens[0],"GET") || !strcmp(tokens[0],"MOD") || !strcmp(tokens[0],"DEL")){
            index += len;
        }
        //操作符不匹配
        else{
            ret += "-ERR OP\r\n";
            index += len;
            return -1;
        }
    }
    //操作符长度不匹配
    else{
        ret += "-ERR OP LEN\r\n";
        return -1;
    }

    return 1;
}



int RespParser::ParseKey(std::string &buffer, int &index, char *tokens[], std::string &ret)
{   
    index = buffer.find('$',index);
    if(index == std::string::npos){
        ret += "-ERR FIND NO";
        return -1;
    }
    index++;

    //解析key长度
    int len = ParseRespNum(buffer,index,ret);
    
    //跳过\r\n  
    SkipCRLF(buffer,index,ret);
    
    tokens[1] = strdup(buffer.substr(index,len).c_str());
    index += len;


    return 0;
}



int RespParser::ParseValue(std::string &buffer, int &index, char *tokens[], std::string &ret)
{
    index = buffer.find('$',index);
    if(index == std::string::npos){
        ret += "-ERR FIND NO";
        return -1;
    }
    index++;

    //解析value长度
    int len = ParseRespNum(buffer,index,ret);

    //跳过\r\n
    SkipCRLF(buffer,index,ret);

    tokens[2] = strdup(buffer.substr(index,len).c_str());
    index += len;

    return 0;
}
