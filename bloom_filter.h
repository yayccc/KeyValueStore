#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <cstdint>

class BloomFilter
{   
private:
    static constexpr size_t BITS_PER_WORD = 64;

    std::vector<uint64_t> bits_aligned_;
    size_t size_;
    size_t hash_count_;

    //设置指定的位置为1
    inline void SetBit(size_t index){
        const size_t word_index = index / BITS_PER_WORD;
        const size_t bit_index = index % BITS_PER_WORD;
        bits_aligned_[word_index] |= (1ULL << bit_index);
    }

    //获取指定位置的值
    inline bool GetBit(size_t index) const{
        const size_t word_index = index / BITS_PER_WORD;
        const size_t bit_index = index % BITS_PER_WORD;
        return (bits_aligned_[word_index] & (1ULL << bit_index)) != 0;
    }   

    //计算多个hash值,返回hash值的数组
    std::vector<size_t> GetHashValues(const std::string &key){
        std::vector<size_t> hash_values;

        size_t hash1 = std::hash<std::string>{}(key);
        size_t hash2 = std::hash<std::string>{}(key + key);

        for(size_t i = 0;i < hash_count_;i++){
            size_t hash = hash1 + i * hash2;
            hash %= size_;
            hash_values.push_back(hash);
        }
        return hash_values;
    }

    // 计算最优的位数组大小和哈希函数个数
    static std::pair<size_t, size_t> OptimalParameters(size_t item_count, double false_positive_rate) {
        // 添加参数验证
        if (item_count == 0 || false_positive_rate <= 0 || false_positive_rate >= 1) {
            return {1024, 3};  // 返回默认值
        }
    
        // 使用 double 进行计算避免溢出
        double ln2 = std::log(2);
        double ln2_squared = ln2 * ln2;
        
        // m = -(n * ln(p)) / (ln(2)^2)
        double optimal_bits = -(static_cast<double>(item_count) * std::log(false_positive_rate)) / ln2_squared;
        
        // k = (m/n) * ln(2)
        double optimal_k = (optimal_bits / static_cast<double>(item_count)) * ln2;
        
        // 限制返回值在合理范围内
        size_t size = std::min(
            static_cast<size_t>(optimal_bits), 
            static_cast<size_t>(1024 * 1024 * 8)  // 最大 1MB
        );
        
        size_t hashes = std::min(
            static_cast<size_t>(optimal_k),
            static_cast<size_t>(20)  // 最多 20 个哈希函数
        );
        return {size, hashes};
    }

public:
    BloomFilter(size_t expected_items, double false_positive_rate = 0.01) {
        auto [optimal_size, optimal_hashes] = OptimalParameters(expected_items, false_positive_rate);
        size_ = optimal_size;
        hash_count_ = optimal_hashes;
        size_t word_count = (size_ + BITS_PER_WORD - 1) / BITS_PER_WORD;
        bits_aligned_.resize(word_count, 0);
    }

    BloomFilter(size_t size, size_t hash_count) : size_(size), hash_count_(hash_count) {
        size_t word_count = size_;        
        bits_aligned_.resize(word_count, 0);
    }
    void Add(const std::string &key ){
        for(auto index : GetHashValues(key)){
            SetBit(index);
        }
    }   

    void LoadBits(const std::vector<uint64_t> &bits){
        bits_aligned_ = bits;
    }

    bool MaybeContains(const std::string &key){    
        // 添加边界检查
        if (bits_aligned_.empty() || hash_count_ == 0) {
            std::cout << "MaybeContains:: Invalid bloom filter state" << std::endl;
            return false;
        }

        auto hash_values = GetHashValues(key);
        if(hash_values.empty()) {
            std::cout << "MaybeContains:: No hash values generated" << std::endl;
            return false;
        }

        for(size_t i = 0; i < hash_values.size(); i++) {
            auto index = hash_values[i];
            if(index >= size_) {
                std::cout << "MaybeContains:: Hash index out of bounds" << std::endl;
                return false;
            }
            if(!GetBit(index)){
                //std::cout << "MaybeContains:: Key definitely not present" << std::endl;
                return false;
            }
        }

        return true;
    }
    
    const std::vector<uint64_t> & GetBitsAligned() const{
        return bits_aligned_;
    }

    size_t GetHashCount() const{
        return hash_count_;
    }

    size_t GetSize() const{
        return size_;
    }

};

