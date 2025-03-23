// #pragma once
// #include <vector>
// #include <unordered_set>
// #include <set>
// #include <map>
// #include <cassert>  // 添加 assert 头文件
// #include <algorithm>
// #include <limits>
// #include "disk.h"
// #include "Object.h"

// #define REP_NUM 3                           // 每个标签需要分配3个不同的磁盘存储副本
// #define PARTITION_ALLOCATION_THRESHOLD 20   // 选择第20个时间片组作为计算标签初始存储需求的基准
// #define SCALE 0.5                           // 选择初始存储需求的比例因子
// #define MAX_PARTITIONS_PER_TAG 10           // 同一个标签在一个磁盘上的最多区间块数

// class Object; // 前向声明   只需要引用，前向声明足够

#pragma once
#include "global.h"

class TagManager {
public:
    // 使用：初始分配
    // 更新：无
    // 初始设定标签映射硬盘时，每个标签映射三个磁盘 M * 3，数值为 硬盘号
    // std::vector<std::vector<int>> tag_disk_mapping;     
    std::vector<std::vector<int>> init_tag_disk;      
    
    // 使用：初始分配
    // 更新：无
    // 初始设定标签映射硬盘时，记录每个标签三个磁盘的区间块数量 M * 3，数值为 区间块数量
    // std::vector<std::vector<int>> tag_partition_num;
    std::vector<std::vector<int>> init_tag_partition_num;
    
    // 使用：在给对象分配三个区间块时使用
    // 更新：当标签使用新区间块时更新，在删除时更新
    // 记录所有硬盘上的每个区间块上的标签分配情况 N * DISK_PARTITIONS 数值为tag号
    std::vector<std::vector<std::unordered_set<int>>> disk_partition_usage_tagkind;  

    // 使用：通过此变量更新 disk_partition_usage_tagkind 和 disk_tag_partition_num
    // 更新：在对象写入区间块时更新
    // 记录每个硬盘上的每个区间块上的标签对象数量
    std::vector<std::vector<std::vector<int>>> disk_partition_usage_tagnum;   


    // 使用：在给对象分配三个区间块时使用
    // 更新：当标签使用新区间块时更新，在删除干净整个标签时更新
    // 每个硬盘上的含有的标签
    // std::vector<std::unordered_set<int>> disk_tag; 
    std::vector<std::unordered_set<int>> disk_tag_kind;           
    
    // 使用：通过该变量更新 disk_tag_kind
    // 更新：当标签使用新区间块时更新，在删除干净整个标签时更新
    // 每个硬盘上所有含有的标签及其区间块数量的映射
    std::vector<std::map<int, int>> disk_tag_partition_num; 
    
    // 使用：在给对象分配三个区间块时使用
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    // 记录每个标签分配的所有硬盘id和上面的区间块id
    // std::vector<std::map<int, std::vector<int>>> tag_to_disk_partitions;
    std::vector<std::map<int, std::vector<int>>> tag_disk_partition;    
    
    // // 使用：待定
    // // 更新：当标签使用新区间块时更新，在删除干净整个标签时更新
    // // 记录每个标签分配的所有硬盘id和区间块id(3行 * 某列)（一定为3行）
    // std::vector<std::vector<std::pair<int, int>>> tag_to_disk_partition_pairs;  

    // 没有标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> zero_tag_partitions;
    // 1个标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> one_tag_partitions;
    // 2个标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> two_tag_partitions;
    // 3个标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> three_tag_partitions;

    TagManager(int M, int N);  

    // 计算每个标签的初始分配磁盘和所需区间块数量
    void calculate_tag_disk_requirement(const std::vector<std::vector<int>>& sum, 
                              const std::vector<std::vector<int>>& conflict_matrix,
                              std::vector<Disk>& disks);
    // 计算所有磁盘的初始分布
    void allocate_tag_disk_requirement(std::vector<Disk>& disks); 

    // 根据删除对象，更新所有标签信息
    void update_tag_info_after_delete(const Object& object);

    // 根据写入对象，更新所有标签信息
    void update_tag_info_after_write(const Object& object);

};

