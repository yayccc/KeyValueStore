#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include "mem_table.h"


// SSTable header
struct SSTableHeader{

    uint32_t magic_number_;
    uint32_t version;
    uint64_t sequence_number_;
    uint64_t count_;
    uint64_t timestamp_;

    char min_key_[128];
    char max_key_[128];

    //uint32_t crc32_;
};

// Block handle
struct IndexEntry{
    uint32_t key_length_;
    std::string key_;
    uint64_t offset_;
    uint64_t size_;
};

// Footer
struct Footer{
    uint32_t data_block_offset_;
    uint32_t index_block_offset_;
    //uint32_t crc32_;
};


class SSTable{
private:


    // SSTable 的魔数,"SSTA" in hex
    static constexpr uint32_t MAGIC_NUMBER = 0x53534154;
    // SSTable 的文件路径
    std::string file_path_;
    // SSTable 的头部信息
    SSTableHeader header_;
    // SSTable 的索引信息
    std::vector<IndexEntry> index_;
    // SSTable 的尾部信息
    Footer footer_;
    //版本号
    static constexpr uint32_t VERSION = 1;
    // 序列号
    uint64_t sequence_number_ = 0;

public:
    SSTable(const std::string& file_path,uint64_t sequence): file_path_(file_path),sequence_number_(sequence) {}

    // 写入 SSTable
    bool Write(const SkipList& data);
    // 读取特定的 key
    std::string Get(const std::string& key);

    bool KeyMayExist(const std::string& key);

private:
    // 写入头部信息
    void WriteHeader(std::ofstream& out,const SkipList& data);
    // 写入数据块
    void WriteDataBlock(std::ofstream& out, const SkipList& data);
    // 写入索引块
    void WriteIndexBlock(std::ofstream& out);
    // 写入尾部信息
    void WriteFooter(std::ofstream& out);
};

