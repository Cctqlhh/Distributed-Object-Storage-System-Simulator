#pragma once
#include <vector>
#include <set>
#include <unordered_map>
#include "disk.h"

#define REP_NUM 3


class TagManager {
private:
    int tag_id;
    std::vector<int> disk_id;
    int reserve_size; // 1/3
    std::vector<int> start_pos; // 3个磁盘的起始位置，后续位置通过+n(0 1 2) size确定
    std::vector<int> end_pos; // 3个磁盘的结束位置 前1/3部分
    std::vector<int> dynamic_end_pos; // 3个磁盘的动态结束位置
    std::vector<std::unordered_map<int, std::vector<int>>> fix_size_free_pos_map; // 3个磁盘，每个磁盘的固定大小空闲位置map
    std::vector<std::unordered_map<int, std::vector<int>>> dynamic_size_free_pos_map; // 随磁头动态

    // std::vector<std::set<std::pair<int, int>>> fix_free_continue_space; // start size
    // 移动到reserve之后时，更新为相应 固定空闲连续空间set+reserve偏移
    // std::vector<std::set<std::pair<int, int>>> dynamic_free_continue_space; // start size
       // std::vector<int> replica_disks;           // 副本所在磁盘ID
    std::vector<std::vector<int>> unit_pos;   // 每个副本的存储单元位置 3个副本，每个副本最多存储size个块
    int last_request_point;
    bool is_deleted;

public:
    // TagManager(int tag_id, int reserve_size, int ) : 
    //     tag_id(tag_id),
    //     reserve_size(reserve_size),
    //     {}
    
    // 刷新令牌（每个时间片调用）
    // void refresh() {
    //     for (int i = 1; i <= disk_num_; i++) {
    //         current_tokens_[i] = max_tokens_;
    //     }
    // } 


};