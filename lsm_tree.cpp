#include "lsm_tree.h"

//插入一个键值对
void LSMTree::Put(const std::string &key, const std::string &value)
{
    //如果活跃的MemTable满了，则创建一个新的MemTable
    if (active_memtable_->GetCurrentSize() + key.size() + value.size() > active_memtable_->GetMaxSize())
    {
        std::cout<<"HandleMemTableFull"<<std::endl;
        HandleMemTableFull();
    }
    //插入到活跃的MemTable中
    std::cout<<"active_memtable::Put"<<std::endl;
    active_memtable_->Put(key, value);

}


//删除一个键值对，插入一个删除标记
void LSMTree::Delete(const std::string &key)
{
    //如果活跃的MemTable满了，则创建一个新的MemTable
    if (active_memtable_->GetCurrentSize() >= active_memtable_->GetMaxSize())
    {
        HandleMemTableFull();
    }
    //插入一个删除标记
    active_memtable_->Delete(key);
}


//获取一个键值对,如果活跃的MemTable中没有，则从冻结的MemTable中查找
//如果还没有，则从SSTable中查找,如果还没有，则返回空字符串
std::string LSMTree::Get(const std::string &key)
{   
    //先在活跃的MemTable中查找
    std::string value = SearchInMemTables(key);
    //在冻结的MemTable中查找
    if(value.empty()){
        value = SearchInImmutableMemTables(key);
    }

    //如果没有找到，则在SSTable中查找
    if(value.empty()){
        value = SearchInSSTables(key);
    }

    if(value.empty()){
        return "NULL";
    }
    else{
        return value;
    }

}


//处理活跃的MemTable满了的情况
void LSMTree::HandleMemTableFull()
{
    //冻结活跃的MemTable
    active_memtable_->Freeze();
    //满了，则创建一个新的MemTable
    immutable_memtables_.push_back(std::move(active_memtable_));
    //创建一个新的活跃的MemTable
    active_memtable_ = std::make_unique<MemTable>(max_memtable_size_);

    //如果冻结的MemTable的数量超过了最大层数，则写入SSTable
    if(immutable_memtables_.size() > IMMUTABLE_MEMTABLES_SIZE){
        //确保L0目录存在
        std::string level_path = GetLevelPath(0);
        if(system(("mkdir -p " + level_path).c_str()) == -1){
            std::cerr<<"mkdir error"<<std::endl;
            return;
        }


        //将冻结的MemTable写入到SSTable中
        while(!immutable_memtables_.empty()){
            int sequence = GetNextSequenceNumber();

            //取出第一个冻结的MemTable
            auto memtable = std::move(immutable_memtables_.front());
            immutable_memtables_.erase(immutable_memtables_.begin());

            //创建新的SSTable路径
            std::string sstable_path = GetSSTablePath(0,sequence);
            //创建新的SSTable
            auto sstable = std::make_unique<SSTable>(sstable_path,sequence);
            std::cout<<"Wirte to sstable_path:"<<sstable_path<<std::endl;

            //将冻结的MemTable中的数据写入到SSTable中
            sstable->Write(memtable->GetTable());
            //将SSTable加入到L0层
            levels_[0].tables.push_back(std::move(sstable));
            std::cout<<"sstable write success"<<std::endl;
        }
        //检查L0层是否需要合并

    }
}



// 启动合并线程
void LSMTree::StartCompactionThread()
{
    compact_thread_ = std::thread([this]() { this->CompactionWorker(); });
}

//合并线程
void LSMTree::CompactionWorker()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

//在活跃的MemTable中查找
std::string LSMTree::SearchInMemTables(const std::string &key)
{
    return active_memtable_->Get(key);
}

std::string LSMTree::SearchInImmutableMemTables(const std::string &key)
{
    std::string value;
    for(auto& memtable : immutable_memtables_){
        value = memtable->Get(key);
        if(!value.empty()){
            return value;
        }
    }
    return "";
}

//在SSTable中查找
std::string LSMTree::SearchInSSTables(const std::string &key)
{
        // 从 L0 开始查找
        for (int level = 0; level < levels_.size(); ++level) {
            // L0 层可能有重叠，需要从新到旧查找所有文件
            if (level == 0) {
                for (auto it = levels_[0].tables.rbegin(); 
                    it != levels_[0].tables.rend(); ++it) {
                    if ((*it)->KeyMayExist(key)) {
                        std::string value = (*it)->Get(key);
                        if (!value.empty()) return value;
                    }
                }
            } 
            // 其他层文件不重叠，可以二分查找
            // else {
            //     auto& tables = levels_[level].tables;
            //     auto it = std::lower_bound(tables.begin(), tables.end(),key, [](const auto& table, const std::string& k) {
            //             return table->GetMaxKey() < k;
            //         });
            //     if (it != tables.end() && (*it)->KeyMayExist(key)) {
            //         std::string value = (*it)->Get(key);
            //         if (!value.empty()) return value;
            //     }
            // }
        }
        return "";
}