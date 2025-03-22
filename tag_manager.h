#pragma once
#include <vector>
#include <unordered_set>
#include "disk.h"
#include "global.h"

#define REP_NUM 3  // 每个标签需要分配3个不同的磁盘存储副本
#define PARTITION_ALLOCATION_THRESHOLD 20  // 选择第20个时间片组作为计算标签初始存储需求的基准

class TagManager {
public:
    std::vector<std::vector<int>> tag_disk_mapping;         // 记录每个标签分配的 3 个磁盘
    std::vector<std::vector<int>> tag_partition_mapping;    // 记录每个标签在所分配磁盘上的区间块数量
    std::vector<std::vector<int>> disk_partition_usage;     // 记录每个磁盘的区间块分配情况
    std::vector<std::vector<double>> tag_delete_prob;      // 记录每个标签的区间块分配情况

    TagManager(int M = 0, int N = 0, int slicing_count = 0);
    // 为每个标签分配磁盘和区间块
    void allocate_tag_storage(const std::vector<std::vector<int>>& sum, 
                              const std::vector<std::vector<int>>& conflict_matrix,
                              std::vector<Disk>& disks);
    // 计算最终磁盘分布
    void allocate_final_storage(std::vector<Disk>& disks); 
    void compute_delete_prob(const std::vector<std::vector<int>>& sum, 
        const std::vector<std::vector<int>>& fre_del);
};