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
    active_memtable_ = std::make_unique<MemTable>(MEMTABLE_SIZE);

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
            //获取level0的序列号
            int sequence = GetNextSequenceNumber(0);
            //取出第一个冻结的MemTable
            auto memtable = std::move(immutable_memtables_.front());
            immutable_memtables_.erase(immutable_memtables_.begin());

            //创建新的SSTable路径
            std::string sstable_path = GetSSTablePath(0,sequence);
            //创建新的SSTable
            auto sstable = std::make_unique<SSTable>(sstable_path,sequence);

            //将冻结的MemTable中的数据写入到SSTable中
            sstable->Write(memtable->GetTable());

            //将SSTable加入到L0层
            levels_[0].tables.push_back(std::move(sstable));

            //更新level0的大小
            levels_[0].size_bytes += memtable->GetCurrentSize();

        }
        //检查是否需要合并
        //CheckCompaction();

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
            //遍历每个SSTable
            else {
                for (auto& table : levels_[level].tables) {
                    if (table->KeyMayExist(key)) {
                        std::string value = table->Get(key);
                        if (!value.empty()) return value;
                    }
                }
            }
        }
        return "";
}


//选择合并候选
LSMTree::CompactionCondidate LSMTree::PickCompaction()
{
    // 1. 检查 L0 文件数量
    if (levels_[0].tables.size() >= 4) {
        return PickL0Compaction();
    }
    // 2. 检查其他层大小
    for (int level = 1; level < max_level_; level++) { 
        //std::cout<<"Compactions:: PickCompaction : level = "<<level<<", size_bytes = "<<levels_[level].size_bytes<<", max_size_bytes = "<<levels_[level].max_size_bytes<<",the level+1 max_size_bytes = "<<levels_[level+1].max_size_bytes<<std::endl;
        if (levels_[level].size_bytes >= levels_[level].max_size_bytes) {
            return PickLevelCompaction(level);
        }
    }
    return {};
}


//选择L0层的合并候选
LSMTree::CompactionCondidate LSMTree::PickL0Compaction()
{
    //std::cout<<"Compaction:: PickL0Compaction"<<std::endl;
    CompactionCondidate compaction_condidate;
    compaction_condidate.level = 0;

    compaction_condidate.smallest_key = levels_[0].tables[0]->GetMinKey();
    compaction_condidate.largest_key = levels_[0].tables[0]->GetMaxKey();

    //将L0的所有文件加入到候选中
    for(auto &table : levels_[0].tables){
        compaction_condidate.tables.push_back(table.get());
        //std::cout<<"Compaction:: PickL0Compaction : table.size() = "<<compaction_condidate.tables.size()<<std::endl;
        if(compaction_condidate.smallest_key.compare(table->GetMinKey()) > 0){
            compaction_condidate.smallest_key = table->GetMinKey();
        }
        if(compaction_condidate.largest_key.compare(table->GetMaxKey()) < 0){
            compaction_condidate.largest_key = table->GetMaxKey();
        }
    }

    //如果L1层有文件，则需要合并
    if(levels_[1].tables.size() > 0){
        compaction_condidate.level = 1;
        compaction_condidate.tables.push_back(levels_[1].tables[0].get());
    }

    return compaction_condidate;
}


//选择某一层的合并候选，非0层级只有一个文件
LSMTree::CompactionCondidate LSMTree::PickLevelCompaction(int level)
{
    if(level == 0){
        return PickL0Compaction();
    }

    CompactionCondidate compaction_condidate;
    compaction_condidate.level = level;

    //找到最小的key
    compaction_condidate.smallest_key = levels_[level].tables[0]->GetMinKey();
    compaction_condidate.largest_key = levels_[level].tables[0]->GetMaxKey();

    compaction_condidate.tables.push_back(levels_[level].tables[0].get());

    return compaction_condidate;
}



//执行合并
void LSMTree::DoCompaction(const CompactionCondidate &compaction)
{   
    std::cout<<"DoCompaction"<<std::endl;
    //获取需要合并的SSTable
    std::vector<SSTable*> input_tables = compaction.tables;

    //如果不是L0层，则需要加入上一层重叠的SSTable
    if(compaction.level > 0){
        // for(auto& table :levels_[compaction.level + 1].tables){
        //     if(table->CheckKeyRange(compaction.smallest_key) 
        //         || table->CheckKeyRange(compaction.largest_key)){
        //         input_tables.push_back(table.get());
        //     }
        // }
        
        //如果下一层存在，则加入下一层的SSTable
        if(levels_[compaction.level + 1].tables.size() > 0){
            input_tables.push_back(levels_[compaction.level + 1].tables[0].get());
        }
    }

    //创建合并后的新文件路径
    int sequence = GetNextSequenceNumber(compaction.level + 1);
    std::string new_sstable_path = GetSSTablePath(compaction.level + 1,sequence);

    std::cout<<"DoCompaction : new_sstable_path = "<<new_sstable_path<<std::endl;
    //如果目录不存在，则创建
    if(system(("mkdir -p " + GetLevelPath(compaction.level + 1)).c_str()) == -1){
        std::cerr<<"mkdir error"<<std::endl;
        return;
    }

    //创建新的SSTable
    auto new_sstable = std::make_unique<SSTable>(new_sstable_path,sequence);

    //合并
    SkipList merged_data(32);
    for(auto& table : input_tables){
        //打印SSTable信息
        //table->Print();

        auto iterator = std::move(table->GetIterator());
        //打印迭代器信息
        //iterator.Print();
        
        iterator.Next();
        if(!iterator.Valid()){
            std::cout<<"DoCompaction:: iterator is empty"<<std::endl;
            continue;
        }
        while(iterator.Valid()){
            //如果key已经存在，则跳过，只保留最新的
            if(merged_data.Search(iterator.key()) != ""){
                iterator.Next();
            }
            else{
                merged_data.Insert(iterator.key(),iterator.value());
                iterator.Next();
            }
        }
        iterator.Close();
    }

    if(merged_data.Size() == 0){
        return;
    }
    //写入新的SSTable
    new_sstable->Write(merged_data);
    
    //将新的SSTable加入到下一层
    levels_[compaction.level + 1].tables.push_back(std::move(new_sstable));


    //更新当前层的大小
    levels_[compaction.level].size_bytes = 0;
    //更新level + 1的大小
    levels_[compaction.level + 1].size_bytes = merged_data.Size(); 

    //删除被合并的SSTable
    for(auto& table : input_tables){
        for(auto& level_table : levels_[compaction.level].tables){
            if(level_table.get() == table){
                level_table->Delete();
                break;
            }
        }
    }

    
    // 移除当前层的SSTable
    auto& current_level_tables = levels_[compaction.level].tables;
    current_level_tables.erase(
        std::remove_if(current_level_tables.begin(), current_level_tables.end(),
            [&input_tables](const auto& table) {
                return std::find(input_tables.begin(), input_tables.end(), table.get()) != input_tables.end();
            }
        ),
        current_level_tables.end()
    );

    // 移除下一层的SSTable
    auto& next_level_tables = levels_[compaction.level + 1].tables;
    next_level_tables.erase(
        std::remove_if(next_level_tables.begin(), next_level_tables.end(),
            [&input_tables](const auto& table) {
                return std::find(input_tables.begin(), input_tables.end(), table.get()) != input_tables.end();
            }
        ),
        next_level_tables.end()
    );

}

void LSMTree::CheckCompaction()
{
    //如果正在合并，则不进行检查
    if(is_compaction_.load()){
        return;
    }
    //选择合并候选
    CompactionCondidate compaction = PickCompaction();
    
    //如果需要合并，则进行合并
    if(compaction.tables.size() > 0){
        is_compaction_.store(true);
        compaction_pool_.Enqueue([this, compaction](){
            DoCompaction(compaction);
            is_compaction_.store(false);
        });
    }
}
