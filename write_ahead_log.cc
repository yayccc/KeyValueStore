//data structure
//| OP (1B) | Key Len (4B) | Key (变长) | Value Len (4B) | Value (变长) | CRC32 (4B) |

#include "write_ahead_log.h"



//0:SET 1:GET 2:MOD 3:DEL
int WriteAheadLog::GetOperate(char *operate){
    if(strcmp(operate,"SET") == 0){
        return 0;
    }
    else if(strcmp(operate,"GET") == 0){
        return 1;
    }
    else if(strcmp(operate,"MOD") == 0){
        return 2;
    }
    else if(strcmp(operate,"DEL") == 0){
        return 3;
    }
    else{
        return -1;
    }
}



WriteAheadLog::WriteAheadLog(){
}



WriteAheadLog::~WriteAheadLog(){
    single_log_file_.close();
}


//以mode模式打开文件，返回是否打开成功
bool WriteAheadLog::OpenSingleLogFile(std::ios::openmode mode)
{
    single_log_file_name_ = SINGLE_LOG_FILE_NAME;

    single_log_file_.open(single_log_file_name_, mode);

    if(single_log_file_.is_open()){
        std::cout<<"log file open success"<<std::endl;
        return true;
    }
    else{
        std::cout<<"log file open failed"<<std::endl;
        return false;
    }

    return false;
}



void WriteAheadLog::CloseSingleLogFile(){
    single_log_file_.close();
}


//从文件以<op> <key> <value>格式恢复数据
void WriteAheadLog::RestoreFromSingleLog(MapEngine &map)
{
    // data structure
    // <op> <key> <value>
    std::string line;
    std::vector<std::string> lines;
    int lines_cnt = 0;
    while(getline(single_log_file_,line)){
        lines.push_back(line);
        lines_cnt ++;
    }
    
    for(int i = 0;i < lines_cnt;i ++){
        ProtocolParser parser;
        char *tokens[MAX_TOKENS];

        char *buffer = (char*)lines[i].c_str();

        int tokens_cnt = parser.ParseReadBuffer(buffer,tokens);

        int operate = map.GetOperate(tokens[0]);
        switch(operate){
            case 0:{
                map.Set(tokens);
                break;                               
            }
            case 2:{
                map.Mod(tokens);
                break;                                
            }
            case 3:{
                map.Del(tokens);
                break;                                
            }
            default:{
                break;                                
            }
        }
        memset(tokens,0,sizeof(tokens));
    }
}



//从文件以op-keylen-key-valuelen-value-crc格式恢复数据
void WriteAheadLog::RestoreFromSingleLog(MapEngine &map, bool enable_crc32)
{   
    //data structure
    //|OP(1B)|Key Len(4B)|Key(变长)|Value Len(4B)|Value(变长)|CRC32(4B)|

    //打开文件
    single_log_file_.open(SINGLE_LOG_FILE_NAME,std::ios::in|std::ios::binary);
    if(single_log_file_.is_open()){
        std::cout<<"log file open success"<<std::endl;
    }
    else{
        std::cout<<"log file open failed"<<std::endl;
        return;
    }

    //将文件指针移动到文件头(二进制文件)
    single_log_file_.seekg(0,std::ios::beg);

    //读取文件,直到文件末尾
    while(!single_log_file_.eof()){

        //读取1B操作码
        uint8_t operate;
        single_log_file_.read((char*)&operate,1);
        if(single_log_file_.gcount() != 1)break;
        std::cout<<"operate:"<<operate<<std::endl;

        //读取key长度
        uint32_t key_len;
        single_log_file_.read((char*)&key_len,KEY_LEN_SIZE);
        if(single_log_file_.gcount() != KEY_LEN_SIZE)break;
        std::cout<<"key_len:"<<key_len<<std::endl;

        //读取key
        char key[key_len + 1];
        single_log_file_.read(key,key_len);
        if(single_log_file_.gcount() != key_len){
            break;
        }
        key[key_len] = '\0';
        std::cout<<"key:"<<key<<std::endl;

        //读取value长度
        uint32_t value_len;
        single_log_file_.read((char*)&value_len,VALUE_LEN_SIZE);
        if(single_log_file_.gcount() != VALUE_LEN_SIZE){
            break; 
        }
        std::cout<<"value_len:"<<value_len<<std::endl;

        //读取value
        char value[value_len + 1];
        single_log_file_.read(value,value_len);
        if(single_log_file_.gcount() != value_len){
            break;
        }
        value[value_len] = '\0';

        
        //读取crc32

        //根据操作码执行操作
        switch(operate){
            case 0:{
                char *tokens[3] = {const_cast<char*>("SET"),key,value};
                map.Set(tokens);
                break;
            }
            case 2:{
                char *tokens[3] = {const_cast<char*>("MOD"),key,value};
                map.Mod(tokens);
                break;
            }
            case 3:{
                char *tokens[2] = {const_cast<char*>("DEL"),key};
                map.Del(tokens);
                break;
            }
            default:{
                break;
            }
        }
    }
    single_log_file_.close();
}



void WriteAheadLog::WriteToSingleLog(char *tokens[], int tokens_cnt, bool enable_crc32)
{   
    //data structure
    //|OP(1B)|Key Len(4B)|Key(变长)|Value Len(4B)|Value(变长)|CRC32(4B)|

    //打开文件
    single_log_file_.open(SINGLE_LOG_FILE_NAME,std::ios::out|std::ios::app|std::ios::binary);
    if(single_log_file_.is_open()){
        std::cout<<"log file open success"<<std::endl;
    }
    else{
        std::cout<<"log file open failed"<<std::endl;
    }

    //写入1B操作码

    uint8_t operate = (uint8_t)GetOperate(tokens[0]);
    single_log_file_.write((char*)&operate,1);

    //写入4B key长度
    uint32_t key_len = strlen(tokens[1]);
    single_log_file_.write((char*)&key_len,KEY_LEN_SIZE);

    //写入key
    single_log_file_.write(tokens[1],key_len);

    //写入4B value长度
    //如果是SET和MOD操作
    if(operate == 0 || operate == 2 ){
        uint32_t value_len = strlen(tokens[2]);
        single_log_file_.write((char*)&value_len,VALUE_LEN_SIZE);
    }
    //如果是DEL和GET操作(无value)
    else if(operate == 3){
        uint32_t value_len = 0;
        single_log_file_.write((char*)&value_len,VALUE_LEN_SIZE);
    }

    //写入value
    if(operate == 0 || operate == 2){
        single_log_file_.write(tokens[2],strlen(tokens[2]));
    }

    single_log_file_.flush();
    single_log_file_.close();
}
