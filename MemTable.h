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

};



//内存表
class MemTable{
private:
    SkipList table; // 或使用红黑树
    size_t max_size;

public:
    MemTable(size_t max_size) : table(16), max_size(max_size) {}

    void Put(const std::string& key, const std::string& value) {
        table.Insert(key, value);
        if (table.Size() >= max_size) {
            Flush();
        }
    }

    std::string Get(const std::string& key) {
        return table.Search(key);
    }

    void Delete(const std::string& key) {
        table.Insert(key, "DELETE"); // 插入删除标记
    }

    void Flush() {
        // 将 MemTable 内容写入磁盘，生成 SSTable
    }
};
    