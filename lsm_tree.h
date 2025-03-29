#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <atomic>

#include "mem_table.h"
#include "sstable.h"


const int MEMTABLE_SIZE = 300; //memtable的默认大小
const int IMMUTABLE_MEMTABLES_SIZE = 1;//冻结的MemTable的最大数量

//LSM树
class LSMTree{
private:
    // 活跃的 MemTable
    std::unique_ptr<MemTable> active_memtable_;
    
    // 已冻结等待写入磁盘的 MemTable
    std::vector<std::unique_ptr<MemTable>> immutable_memtables_;

    size_t max_memtable_size_;         // MemTable 最大大小
    
    std::string db_path_;              // 数据库路径

    int max_level_ = 7;                    // LSM树最大层数

    struct level{
        std::vector<std::unique_ptr<SSTable>> tables;
        size_t size_bytes = 0; // SSTable 总大小
        size_t max_size_bytes = 1024 * 1024; // SSTable 最大大小
    };

    std::vector<level> levels_; // 各层 SSTable

    //序列号计数器
    std::atomic<uint64_t> sequence_number_{0};

    // 后台合并线程
    bool running_ = false;
    std::thread compact_thread_;

public:
    //带参数的构造函数
    //db_path为数据库路径,max_memtable_size为MemTable最大大小,max_level为LSM树最大层数
    LSMTree(const std::string& db_path, 
            size_t max_memtable_size = MEMTABLE_SIZE, // 默认1MB 
            int max_level = 7) 
        : db_path_(db_path)
        , max_memtable_size_(max_memtable_size)
        , max_level_(max_level) {
        // 初始化活跃 memtable
        active_memtable_ = std::make_unique<MemTable>(max_memtable_size);
        // 启动后台合并线程
        //StartCompactionThread();

        //初始化level
        levels_.resize(max_level_);
    }

    //不带参数的构造函数
    LSMTree(){
        // 默认路径
        db_path_ = "./db";
        // 默认最大大小
        max_memtable_size_ = MEMTABLE_SIZE;
        // 默认最大层数
        max_level_ = 7;
        // 初始化活跃 memtable
        active_memtable_ = std::make_unique<MemTable>(max_memtable_size_);
        // 启动后台合并线程
        //StartCompactionThread();

        //初始化level
        levels_.resize(max_level_);
    }

    // 基本操作接口
    void Put(const std::string& key, const std::string& value);
    std::string Get(const std::string& key);
    void Delete(const std::string& key);


 // 内部方法
private:
   
    //处理活跃的MemTable满了的情况
    void HandleMemTableFull();
    // 启动合并线程
    void StartCompactionThread();
    //合并线程
    void CompactionWorker();
    
    // 查找相关
    std::string SearchInMemTables(const std::string& key);
    std::string SearchInImmutableMemTables(const std::string& key);
    std::string SearchInSSTables(const std::string& key);

    //获取SSTable的路径
    std::string GetSSTablePath(int level, int index){
        return db_path_ + "/level_" + std::to_string(level) + "/sstable_" + std::to_string(index) + ".sst";
    }

    //获取层级路径
    std::string GetLevelPath(int level){
        return db_path_ + "/level_" + std::to_string(level);
    }

    //获取新的序列号
    uint64_t GetNextSequenceNumber(){
        return sequence_number_++;
    }
};