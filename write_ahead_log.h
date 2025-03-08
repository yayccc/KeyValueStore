#pragma once

#include <string.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "protocol_parser.h"
#include "map_engine.h"

//单个日志文件名
const std::string SINGLE_LOG_FILE_NAME = "log.txt";

//是否启用CRC32校验
const bool ENABLE_CRC32 = false;
//key的长度值的字节数
const int KEY_LEN_SIZE = 4;
//value的长度值的字节数
const int VALUE_LEN_SIZE = 4;


class WriteAheadLog{
public:
    WriteAheadLog();
    ~WriteAheadLog();

    //0:SET 1:GET 2:MOD 3:DEL
    int GetOperate(char *operate);
    
    //以mode模式打开文件，返回是否打开成功
    bool OpenSingleLogFile(std::ios::openmode mode);

    //关闭文件
    void CloseSingleLogFile();

    //从单个日志文件以<op> <key> <value>恢复
    void RestoreFromSingleLog(MapEngine &map);

    //从单个日志文件以WAL格式恢复
    void RestoreFromSingleLog(MapEngine &map,bool enable_crc32);

    //以WAL格式写入单个日志文件
    void WriteToSingleLog(char *tokens[],int tokens_cnt,bool enable_crc32);

private:
    //单个日志文件名
    std::string single_log_file_name_ = SINGLE_LOG_FILE_NAME;
    //单个日志文件
    std::fstream single_log_file_;
};
