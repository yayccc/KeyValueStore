#pragma once

#include <string.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "protocol_parser.h"
#include "map_engine.h"


const std::string SINGLE_LOG_FILE_NAME = "log.txt";
const bool ENABLE_CRC32 = false;

const int KEY_LEN_SIZE = 4;
const int VALUE_LEN_SIZE = 4;


class WriteAheadLog{
public:
    WriteAheadLog();
    ~WriteAheadLog();

    //0:SET 1:GET 2:MOD 3:DEL
    int GetOperate(char *operate);
    
    bool OpenSingleLogFile(std::ios::openmode mode);

    void CloseSingleLogFile();


    bool RestoreFromSingleLog(MapEngine &map);

    void RestoreFromSingleLog(MapEngine &map,bool enable_crc32);
    
    bool WriteToSingleLog(char *buffer,bool enable_crc32);

    void WriteToSingleLog(char *tokens[],int tokens_cnt,bool enable_crc32);
private:
    std::string single_log_file_name_ = SINGLE_LOG_FILE_NAME;
    std::fstream single_log_file_;
};
