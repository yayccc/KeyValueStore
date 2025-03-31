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
#include <map>


#include "mem_table.h"
#include "sstable.h"
#include "threadpool.h"

const int MEMTABLE_SIZE = 512*512; //memtable的默认大小
const int IMMUTABLE_MEMTABLES_SIZE = 1;//冻结的MemTable的最大数量
const int MAX_LEVEL = 7; //LSM树的最大层数
const int COMPACTION_THREAD_NUM = 2; //合并线程的数量


//LSM树
class LSMTree{
private:
    // 活跃的 MemTable
    std::unique_ptr<MemTable> active_memtable_;
    
    // 已冻结等待写入磁盘的 MemTable
    std::vector<std::unique_ptr<MemTable>> immutable_memtables_;
    
    std::string db_path_;              // 数据库路径

    int max_level_ = 7;                    // LSM树最大层数

    struct level{
        std::vector<std::unique_ptr<SSTable>> tables;
        size_t size_bytes = 0; // SSTable 总大小
        size_t max_size_bytes = MEMTABLE_SIZE; // SSTable 最大大小
    };

    std::vector<level> levels_; // 各层 SSTable

    // //序列号计数器
    // std::atomic<uint64_t> sequence_number_{0};
    // //level0的序列号计数器
    // std::atomic<uint64_t> level0_sequence_number_{0};

    // 序列号计数器
    std::atomic<uint64_t> sequence_number_[MAX_LEVEL] = {0};

    ThreadPool compaction_pool_;

    struct CompactionCondidate{
        int level;
        std::vector<SSTable*> tables;
        std::string smallest_key;
        std::string largest_key;
    };

    static constexpr size_t L0_COMPACTION_SIZE = 4;
    static constexpr double LEVEL_COMPACTION_RATIO = 3.0;

    std::atomic<bool> is_compaction_{false}; 
    
public:
    //带参数的构造函数
    //db_path为数据库路径,max_memtable_size为MemTable最大大小,max_level为LSM树最大层数
    LSMTree(const std::string& db_path, 
            size_t max_memtable_size = MEMTABLE_SIZE, // 默认1MB 
            int max_level = 7) 
        : db_path_(db_path)
        , max_level_(max_level)
        , compaction_pool_(COMPACTION_THREAD_NUM){

        // 初始化活跃 memtable
        active_memtable_ = std::make_unique<MemTable>(max_memtable_size);

        //初始化level
        levels_.resize(max_level_);

        //初始化level的大小
        InitLevelsSize();

        //定期检查合并
        PeriodicCompaction();
    }

    //不带参数的构造函数
    LSMTree():compaction_pool_(COMPACTION_THREAD_NUM){
        // 默认路径
        db_path_ = "./db";
        // 默认最大层数
        max_level_ = 7;
        // 初始化活跃 memtable
        active_memtable_ = std::make_unique<MemTable>(MEMTABLE_SIZE);

        //初始化level
        levels_.resize(max_level_);

        //初始化level的大小
        InitLevelsSize();
        
        //定期检查合并
        PeriodicCompaction();
    }

    // 基本操作接口
    //插入
    void Put(const std::string& key, const std::string& value);
    //查询
    std::string Get(const std::string& key);
    //删除
    void Delete(const std::string& key);


 // 内部方法
private:
   
    //处理活跃的MemTable满了的情况
    void HandleMemTableFull();
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
    uint64_t GetNextSequenceNumber(int level){
        if(level == 0){
            return sequence_number_[level]++;
        }
        else{
            return sequence_number_[level];
        }
    }

    //选择合并候选
    CompactionCondidate PickCompaction();
    //选择L0层的合并候选
    CompactionCondidate PickL0Compaction();
    //选择某一层的合并候选
    CompactionCondidate PickLevelCompaction(int level);

    //合并
    void DoCompaction(const CompactionCondidate& compaction);

    //检查是否需要合并
    void CheckCompaction();

    //定期检查合并
    void PeriodicCompaction(){
        compaction_pool_.Enqueue([this](){
            while(true){
                if(is_compaction_.load()){
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
                else{
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cout<<"-----------CheckCompaction ----------"<<std::endl;
                    CheckCompaction();
                }
            }
        });
    }

    //初始化levels大小
    void InitLevelsSize(){
        for(int level = 0; level < max_level_; level++){
            if(level == 0){ 
                levels_[level].max_size_bytes = MEMTABLE_SIZE;
            }
            else{
                levels_[level].max_size_bytes = levels_[level - 1].max_size_bytes * LEVEL_COMPACTION_RATIO;
                std::cout<<"InitLevelsSize:: level = "<<level<<", max_size_bytes = "<<levels_[level].max_size_bytes<<std::endl;
            }
        }
    }
};