#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <random>


//跳表
class SkipList{
private:
    //跳表节点
    struct SkiplistNode{
        std::string key;
        std::string value;
        std::vector<SkiplistNode*> forward;
        SkiplistNode(std::string key, std::string value, int level):key(key),value(value),forward(level+1,nullptr){};
    };
    //头节点
    SkiplistNode* head_;
    //最大层数
    int max_level_;
    //当前跳表的最大层数
    int level_ = 0;
    //跳表的大小
    int skiplist_size_ = 0;
    //键值对的数量
    int count_ = 0;
public:
    SkipList(int max_level);
    ~SkipList(){};
    //插入一个键值对
    void Insert(std::string key, std::string value);
    //删除一个键值对
    void Delete(std::string key);
    //查找一个键值对
    std::string Search(std::string key);
    //打印跳表
    void Print();
    //随机生成一个层数
    int random_level();       
    //返回跳表的大小
    int Size() { return skiplist_size_; }

    int Count() { return count_; }

    //获取跳表的键值对
    std::vector<std::pair<std::string,std::string>> GetData() const{
        std::vector<std::pair<std::string,std::string>> data;
        SkiplistNode* current = head_->forward[0];
        while(current != nullptr){
            data.push_back(std::make_pair(current->key,current->value));
            current = current->forward[0];
        }
        return data;
    }

    std::vector<std::string> GetKeys() const{
        std::vector<std::string> keys;
        SkiplistNode* current = head_->forward[0];
        while(current != nullptr){
            keys.push_back(current->key);
            current = current->forward[0];
        }
        return keys;
    }

};



//内存表
class MemTable{
private:
    SkipList table; // 跳表
    //最大大小
    size_t max_size;
    //当前大小
    size_t current_size = 0;
    //冻结
    bool is_frozen = false;

    //磁盘文件
    std::string disk_file_path;
    //磁盘文件名
    std::string disk_file_name;

    //SSTable序列号
    int sstable_sequence_number = 0;

public:
    //带参数的构造函数
    //max_size为MemTable最大大小
    MemTable(size_t max_size) : table(16), max_size(max_size) {}

    //不带参数的构造函数
    //MemTable() : table(16), max_size(1024 * 1024) {}

    //获取跳表
    SkipList GetTable() {
        return table;
    }

    //插入一个键值对
    void Put(const std::string& key, const std::string& value) {
        table.Insert(key, value);
        current_size = table.Size();
        if (current_size > max_size) {
            Flush();
        }
    }

    //获取一个键值对
    std::string Get(const std::string& key) {
        return table.Search(key);
    }

    //删除一个键值对
    void Delete(const std::string& key) {
        table.Insert(key, "DELETE"); // 插入删除标记
    }

    //刷新
    void Flush() {
        // 将 MemTable 内容写入磁盘，生成 SSTable
        //如果未冻结，则冻结
        if(!is_frozen){
            Freeze();
        }
        // 1. 将 MemTable 内容写入磁盘
        // 2. 生成 SSTable
        // 3. 更新 MemTable 状态
        // 4. 更新当前大小
    }

    //冻结
    void Freeze() {
        is_frozen = true;
    }

    //解冻
    void Unfreeze() {
        is_frozen = false;
    }

    //获取当前大小
    size_t GetCurrentSize() {
        return current_size;
    }
    
    //获取最大大小
    size_t GetMaxSize() {
        return max_size;
    }

    //获取冻结状态
    bool GetIsFrozen() {
        return is_frozen;
    }

private:
    //写入磁盘
    void WriteToDisk();
    //从磁盘读取
    void ReadFromDisk();
};
    