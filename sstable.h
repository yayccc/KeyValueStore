#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>

#include "mem_table.h"
#include "bloom_filter.h"

// SSTable 的魔数,"SSTA" in hex
static constexpr uint32_t MAGIC_NUMBER = 0x53534154;

// SSTable header
struct SSTableHeader{
    //魔数
    uint32_t magic_number_;
    //版本号
    uint32_t version;
    //序列号
    uint64_t sequence_number_;
    //记录的总数量
    uint64_t count_;
    //时间戳
    uint64_t timestamp_;
    //最小key
    char min_key_[128];
    //最大key
    char max_key_[128];

    //crc32校验码
    //uint32_t crc32_;

};

// Block handle
struct SSTableIndexEntry{
    uint32_t key_length_;
    std::string key_;
    uint64_t offset_;
    uint64_t size_;
};

// Footer
struct SSTableFooter{
    uint32_t data_block_offset_;
    uint32_t index_block_offset_;
    uint32_t bloom_filter_offset_;

    //uint32_t crc32_;
};


class SSTableIterator;

class SSTable{
private:


    // SSTable 的文件路径
    std::string file_path_;
    // SSTable 的头部信息
    SSTableHeader header_;
    // SSTable 的索引信息
    std::vector<SSTableIndexEntry> index_;
    // SSTable 的尾部信息
    SSTableFooter footer_;
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
    // 检查 key 是否可能存在
    bool KeyMayExist(const std::string& key);
    //检查key的范围
    bool CheckKeyRange(const std::string& key);

    //获取最小key
    std::string GetMinKey(){
        return header_.min_key_;
    }
    //获取最大key
    std::string GetMaxKey(){
        return header_.max_key_;
    }

    //获取迭代器
    SSTableIterator GetIterator();

    //删除SSTable
    void Delete();

    //打印SSTable信息
    void Print() const{
        std::cout<<"SSTable:: file_path_ = "<<file_path_<<std::endl;
        std::cout<<"SSTable:: header_.count_ = "<<header_.count_<<std::endl;
        std::cout<<"SSTable:: header_.min_key_ = "<<header_.min_key_<<std::endl;
        std::cout<<"SSTable:: header_.max_key_ = "<<header_.max_key_<<std::endl;

        std::cout<<"SSTable:: footer_.data_block_offset_ = "<<footer_.data_block_offset_<<std::endl;
        std::cout<<"SSTable:: footer_.index_block_offset_ = "<<footer_.index_block_offset_<<std::endl;
        
    }
private:
    // 写入头部信息
    void WriteHeader(std::ofstream& out,const SkipList& data);
    // 写入数据块
    void WriteDataBlock(std::ofstream& out, const SkipList& data);
    // 写入索引块
    void WriteIndexBlock(std::ofstream& out);
    // 写入尾部信息
    void WriteFooter(std::ofstream& out);

    //写入布隆过滤器
    void WriteBloomFilter(std::ofstream& out,const SkipList& data);


};

//SSTable迭代器
class SSTableIterator{
private:
    // SSTable 的文件路径
    std::string file_path_;
    //文件流
    std::ifstream file_;

    //当前迭代器的位置
    uint32_t current_position_;

    //数据块偏移量
    uint32_t data_block_offset_;
    //数据块末尾偏移量
    uint32_t data_block_end_offset_;
    //索引块偏移量
    uint32_t index_block_offset_;
    //布隆过滤器偏移量
    uint32_t bloom_filter_offset_;

    //当前读取的key
    std::string current_key_;

    //当前读取的value
    std::string current_value_;
    
    //记录的总数量
    uint64_t total_count_;

    //当前已经读取的数量
    uint64_t current_count_;

    //是否已经读取到尾部
    bool is_end_;

public:
    SSTableIterator(const std::string& file_path);
    SSTableIterator(const std::string& file_path,const SSTableHeader& header,const SSTableFooter& footer){
        file_path_ = file_path;
        total_count_ = header.count_;
        data_block_offset_ = footer.data_block_offset_;
        data_block_end_offset_ = footer.index_block_offset_;
        index_block_offset_ = footer.index_block_offset_;
        bloom_filter_offset_ = footer.bloom_filter_offset_;


        current_position_ = data_block_offset_;
        current_count_ = 0;
        is_end_ = false;
        current_key_ = "";
        current_value_ = "";
        file_.open(file_path_,std::ios::binary);
        if(!file_.is_open()){
            std::cerr<<"SSTableIterator:: file open failed"<<std::endl;
        }
        file_.seekg(data_block_offset_);
    };


    void Close(){
        file_.close();
    }
    
    //迭代器操作
    //判断是否有效
    bool Valid() const;
    //获取key
    std::string key() const;
    //获取value
    std::string value() const;
    //移动到下一个
    void Next();

    //设置文件路径
    void SetFilePath(const std::string& file_path){
        file_path_ = file_path;
    }
    //获取文件路径
    std::string GetFilePath() const{
        return file_path_;
    }

    //打印迭代器信息
    void Print() const{
        std::cout<<"SSTableIterator:: file_path_ = "<<file_path_<<std::endl;
        std::cout<<"SSTableIterator:: current_position_ = "<<current_position_<<std::endl;
        std::cout<<"SSTableIterator:: total_count_ = "<<total_count_<<std::endl;
        std::cout<<"SSTableIterator:: current_count_ = "<<current_count_<<std::endl;
        std::cout<<"SSTableIterator:: is_end_ = "<<is_end_<<std::endl;
    }


private:
    //读取头部信息
    bool ReadHeader();
    //读取footer
    bool ReadFooter();

};

