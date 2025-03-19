#include "MemTable.h"



SkipList::SkipList(int max_level) : max_level_(max_level)
{
    head_ = new SkiplistNode("","",max_level);
}



void SkipList::Insert(std::string key, std::string value)
{   
    //update[i]记录每一层小于key的最大节点
    std::vector<SkiplistNode*> update(max_level_, nullptr);
    //当前节点
    SkiplistNode* current = head_;

    //从最高层开始查找
    for(int i = level_;i >= 0;i --){
        while(current->forward[i] != nullptr && current->forward[i]->key < key){
            current = current->forward[i];
        }
        update[i] = current;
    }

    //当前节点指向小于等于key的最大节点
    current = current->forward[0];

    //如果key已经存在，则更新value,同时更新跳表大小
    if(current != nullptr && current->key == key){
        //更新跳表大小
        skiplist_size_ += (value.length() - current->value.length());
        //更新value
        current->value = value;
        return;
    }
    else{
        //更新跳表大小
        skiplist_size_ += (key.length() + value.length());
    }

    //随机生成一个层数
    int new_level = random_level();
    //如果新节点的层数大于当前跳表的层数，则更新update数组
    if(new_level > level_){
        for(int i = level_ + 1;i <= new_level;i++){
            update[i] = head_;
        }
        level_ = new_level;
    }
    //创建新节点
    SkiplistNode* new_node = new SkiplistNode(key,value,new_level);
    //更新每一层的指针
    for(int i = 0;i <= new_level;i++){
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

}



void SkipList::Delete(std::string key)
{   
    //update[i]记录每一层小于key的最大节点
    SkiplistNode* update[max_level_ + 1];
    //当前节点
    SkiplistNode* current = head_;
    //从最高层开始查找
    for(int i = level_;i >= 0;i --){
        while(current->forward[i] != nullptr && current->forward[i]->key < key){
            current = current->forward[i];
        }
        update[i] = current;
    }
    //当前节点指向小于等于key的最大节点
    current = current->forward[0];
    //如果key存在，则删除
    if(current != nullptr && current->key == key){
        for(int i = 0;i <= level_;i++){
            if(update[i]->forward[i] != current)break;
            update[i]->forward[i] = current->forward[i];
        }
        delete current;
        while(level_ > 0 && head_->forward[level_] == nullptr){
            level_--;
        }
        //更新跳表大小
        skiplist_size_ -= (key.length() + current->value.length());
    }
}



std::string SkipList::Search(std::string key)
{   
    //当前节点
    SkiplistNode* current = head_;
    //从最高层开始查找
    for(int i = level_;i >= 0;i --){
        while(current->forward[i] != nullptr && current->forward[i]->key < key){
            current = current->forward[i];
        }
    }
    //当前节点指向小于等于key的最大节点
    current = current->forward[0];
    //如果key存在，则返回value
    if(current != nullptr && current->key == key){
        return current->value;
    }else{
        return "";
    }
}



void SkipList::Print()
{
    
    std::cout << "Skip List (Level: " << level_ << "):" << std::endl;
    for (int i = level_ - 1; i >= 0; i--) {
        SkiplistNode* current = head_->forward[i];
        std::cout << "Level " << i << ": ";
        while (current != nullptr) {
            std::cout<<current->key<<"->"<<current->value<<" ";
            current = current->forward[i];
        }
        std::cout <<std::endl;
    }
}



int SkipList::random_level()
{
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    int ret_level = 0;
    while(dis(gen) < 0.5 && ret_level < max_level_ - 1){
        ret_level++;
    }
    return ret_level;
}
