#include "sstable.h"


bool SSTable::Write(const SkipList &data)
{   
    std::ofstream out(file_path_, std::ios::binary | std::ios::out);
    if (!out)
    {
        std::cerr << "Failed to open file: " << file_path_ << std::endl;
        return false;
    }

    // 写入头部信息
    WriteHeader(out,data);
    //写入布隆过滤器
    WriteBloomFilter(out,data);
    // 写入数据块
    WriteDataBlock(out, data);
    // 写入索引块
    WriteIndexBlock(out);   
    // 写入尾部信息
    WriteFooter(out);

    out.close();
    return true;
}

// 写入头部信息
void SSTable::WriteHeader(std::ofstream &out,const SkipList &data)
{
    //获取数据
    std::vector<std::pair<std::string,std::string>> table_data = data.GetData();

    header_.magic_number_ = MAGIC_NUMBER;
    header_.version = VERSION;
    header_.sequence_number_ = sequence_number_++;
    header_.count_ = table_data.size();
    header_.timestamp_ = time(nullptr);

    strncpy(header_.min_key_,table_data.front().first.c_str(),sizeof(header_.min_key_));
    strncpy(header_.max_key_,table_data.back().first.c_str(),sizeof(header_.max_key_));

    // 写入头部信息
    out.write(reinterpret_cast<const char*>(&header_),sizeof(header_));
}


// 写入数据块
void SSTable::WriteDataBlock(std::ofstream &out, const SkipList &data)
{
    // 获取当前写指针位置
    uint32_t current_offset = out.tellp();

    // 记录数据块偏移量
    footer_.data_block_offset_ = current_offset;

    // 写入数据块，遍历跳表的键值对
    for (const auto &item : data.GetData())
    {
        //记录索引信息
        IndexEntry index_entry;
        index_entry.key_ = item.first;
        index_entry.key_length_ = item.first.size();
        index_entry.offset_ = current_offset;

        // 写入key长度
        uint32_t key_length = item.first.size();
        out.write(reinterpret_cast<const char*>(&key_length),sizeof(key_length));

        // 写入key
        out.write(item.first.c_str(),key_length);

        // 写入value长度
        uint32_t value_length = item.second.size();
        out.write(reinterpret_cast<const char*>(&value_length),sizeof(value_length));

        // 写入value
        out.write(item.second.c_str(),value_length);
        
        // 更新当前写指针位置
        current_offset = out.tellp();

        // 更新索引信息
        index_entry.size_ = current_offset - index_entry.offset_;
        index_.push_back(index_entry);

        // 更新当前写指针位置
        current_offset = out.tellp();
    }

}


// 写入索引块
void SSTable::WriteIndexBlock(std::ofstream &out){
    // 记录索引块的偏移量
    footer_.index_block_offset_ = out.tellp();

    // 写入索引块
    for(auto &item : index_){
        out.write(reinterpret_cast<const char*>(&item.key_length_),sizeof(item.key_length_));
        out.write(item.key_.c_str(),item.key_.size());
        out.write(reinterpret_cast<const char*>(&item.offset_),sizeof(item.offset_));
        out.write(reinterpret_cast<const char*>(&item.size_),sizeof(item.size_));
    }
}


// 写入尾部信息
void SSTable::WriteFooter(std::ofstream &out){
    // 写入尾部信息
    out.write(reinterpret_cast<const char*>(&footer_),sizeof(footer_));
}



void SSTable::WriteBloomFilter(std::ofstream &out, const SkipList &data)
{
    // 记录布隆过滤器的偏移量
    footer_.bloom_filter_offset_ = out.tellp();
    // 创建布隆过滤器
    const auto &keys_data = data.GetKeys();
    BloomFilter bloom_filter(keys_data.size());

    // 将数据写入布隆过滤器
    for (const auto &item : keys_data)
    {
        bloom_filter.Add(item);
    }

    //拿到布隆过滤器的位数组，大小，hash函数个数
    const auto &bits = bloom_filter.GetBitsAligned();
    size_t bits_size = bits.size() * sizeof(uint64_t);
    size_t hash_count = bloom_filter.GetHashCount();

    //写入元数据
    out.write(reinterpret_cast<const char*>(&bits_size),sizeof(bits_size));
    out.write(reinterpret_cast<const char*>(&hash_count),sizeof(hash_count));

    //写入位数组
    out.write(reinterpret_cast<const char*>(&bits[0]),bits_size);
}



// 读取特定的key
std::string SSTable::Get(const std::string &key){
    std::ifstream in(file_path_,std::ios::binary);

    if(!in){
        std::cerr << "Failed to open file: " << file_path_ << std::endl;
        return "";
    }
    std::cout<<"SSTable::Get key"<<std::endl;
    // 读取头部信息
    SSTableHeader header;
    in.read(reinterpret_cast<char*>(&header),sizeof(header));
    // 检查魔数
    if(header.magic_number_ != MAGIC_NUMBER){
        std::cerr << "Invalid SSTable file: " << file_path_ << std::endl;
        return "";
    }

    //读取footer
    Footer footer;
    in.seekg(-sizeof(footer),std::ios::end);
    in.read(reinterpret_cast<char*>(&footer),sizeof(footer));

    //读取对应索引
    IndexEntry target_index;
    in.seekg(footer.index_block_offset_,std::ios::beg);
    bool found = false;
    

    for(int i = 0;i < header.count_;i++){
        uint32_t key_length;
        in.read(reinterpret_cast<char*>(&key_length),sizeof(key_length));   
        target_index.key_ = std::string(key_length,'\0');
        in.read(&target_index.key_[0],key_length);
        in.read(reinterpret_cast<char*>(&target_index.offset_),sizeof(target_index.offset_));
        in.read(reinterpret_cast<char*>(&target_index.size_),sizeof(target_index.size_));


        if(strcmp(target_index.key_.c_str(),key.c_str()) == 0){
            found = true;
            break;
        }
    }

    //如果没找到
    if(!found){
        return "";
    }

    //找到索引了，读取对应的数据
    in.seekg(target_index.offset_,std::ios::beg);
    //读取key长度
    uint32_t key_length;
    in.read(reinterpret_cast<char*>(&key_length),sizeof(key_length));
    //读取key
        
    std::string read_key(key_length,'\0');
    in.read(&read_key[0],key_length);
    //读取value长度
    uint32_t value_length;
    in.read(reinterpret_cast<char*>(&value_length),sizeof(value_length));
    //读取value
    std::string ret_value(value_length,'\0');
    in.read(&ret_value[0],value_length);

    return ret_value;
}



bool SSTable::CheckKeyRange(const std::string &key)
{
    // 读取头部信息
    SSTableHeader header;
    std::ifstream in(file_path_, std::ios::binary);
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    std::string min_key(header.min_key_);
    std::string max_key(header.max_key_);
    //如果没有找到
    if(strcmp(key.c_str(),min_key.c_str()) < 0 || strcmp(key.c_str(),max_key.c_str()) > 0){
        return false;
    }
    else{
        return true;
    }
}



// 检查key是否可能存在
bool SSTable::KeyMayExist(const std::string &key)
{   
    // 读取头部信息并检查key范围
    if(!CheckKeyRange(key)){
        return false;
    }

    // 读取布隆过滤器
    std::ifstream in(file_path_,std::ios::binary);
    if(!in){
        std::cerr << "Failed to open file: " << file_path_ << std::endl;
        return false;
    }

    //读取footer
    Footer footer;
    in.seekg(-sizeof(footer),std::ios::end);
    in.read(reinterpret_cast<char*>(&footer),sizeof(footer));

    // 定位到布隆过滤器
    in.seekg(footer.bloom_filter_offset_,std::ios::beg);

    // 读取布隆过滤器
    size_t bits_size;
    size_t hash_count;
    in.read(reinterpret_cast<char*>(&bits_size),sizeof(bits_size));
    in.read(reinterpret_cast<char*>(&hash_count),sizeof(hash_count));

    // 读取位数组
    BloomFilter bloom_filter(header_.count_);
    std::vector<uint64_t> bits(bits_size / sizeof(uint64_t));
    in.read(reinterpret_cast<char*>(&bits[0]),bits_size);
    bloom_filter.LoadBits(bits);

    std::cout<<"KeyMayExist:: find in bloomfilter"<<std::endl;
    // 检查key是否存在
    return bloom_filter.MaybeContains(key);
}
