
#pragma once
#include <vector>
#include <unordered_set>
#include "disk.h"
#include "global.h"

// class Object; // 前向声明   只需要引用，前向声明足够

#pragma once
#include "global.h"

class TagManager {
public:
    std::vector<std::vector<double>> tag_delete_prob;      // 记录每个标签的区间块分配情况

    // 使用：在给对象分配三个区间块时使用
    // 更新：当标签使用新区间块时更新，在删除时更新
    // 记录所有硬盘上的每个区间块上的标签分配情况 N * DISK_PARTITIONS 数值为tag号
    // std::vector<std::vector<std::unordered_set<int>>> disk_partition_usage_tagkind;  
    std::vector<std::vector<std::set<int>>> disk_partition_usage_tagkind;  

    // 使用：通过此变量更新 disk_partition_usage_tagkind 和 disk_tag_partition_num
    // 更新：在对象写入区间块时更新
    // 记录每个硬盘上的每个区间块上的标签对象数量
    std::vector<std::vector<std::vector<int>>> disk_partition_usage_tagnum;   


    // 使用：在给对象分配三个区间块时使用
    // 更新：当标签使用新区间块时更新，在删除干净整个标签时更新
    // 每个硬盘上的含有的标签
    // std::vector<std::unordered_set<int>> disk_tag; 
    // std::vector<std::unordered_set<int>> disk_tag_kind;
    std::vector<std::set<int>> disk_tag_kind;             
    
    // 使用：通过该变量更新 disk_tag_kind
    // 更新：当标签使用新区间块时更新，在删除干净整个标签时更新
    // 每个硬盘上所有含有的标签及其区间块数量的映射
    std::vector<std::map<int, int>> disk_tag_partition_num; // 其中的map基于标签号升序排列
    
    // 使用：在给对象分配三个区间块时使用
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    // 记录每个标签分配的所有硬盘id和上面的区间块id
    // std::vector<std::map<int, std::vector<int>>> tag_to_disk_partitions;
    // std::vector<std::map<int, std::vector<int>>> tag_disk_partition; // 其中的map基于硬盘号升序排列 
    std::vector<std::map<int, std::set<int>>> tag_disk_partition; // 其中的map基于硬盘号升序排列,其次基于区间块号升序排列
    
    // // 使用：待定
    // // 更新：当标签使用新区间块时更新，在删除干净整个标签时更新
    // // 记录每个标签分配的所有硬盘id和区间块id(3行 * 某列)（一定为3行）
    // std::vector<std::vector<std::pair<int, int>>> tag_to_disk_partition_pairs;  

    // 没有标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> zero_tag_partitions; // 其中set首先基于硬盘号升序排列,其次基于区间块号升序排列
    // 1个标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> one_tag_partitions; // 其中set基于硬盘号升序排列,其次基于区间块号升序排列
    // 2个标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> two_tag_partitions; // 其中set基于硬盘号升序排列,其次基于区间块号升序排列
    // 3个标签的磁盘id，区间块id
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> three_tag_partitions; // 其中set基于硬盘号升序排列,其次基于区间块号升序排列

    // 4个及以上标签的磁盘id，区间块id
    // 加快兜底策略遍历速度
    // 当标签使用新区间块时更新，在删除干净整个标签时更新
    std::set<std::pair<int, int>> more_tag_partitions; // 其中set基于硬盘号升序排列,其次基于区间块号升序排列 

    // // 记录每个硬盘每个区间块的对象id
    // std::vector<std::vector<std::vector<int>>> disk_partition_object;

    
    TagManager(int M = 0, int N = 0, int slicing_count = 0);

    // 初始化
    void init(const std::vector<std::vector<int>>& sum, const std::vector<std::vector<int>>& conflict_matrix, std::vector<Disk>& disks);

    // 根据删除对象，更新所有标签信息
    void update_tag_info_after_delete(const Object& object);

    // 根据写入对象，更新所有标签信息
    void update_tag_info_after_write(const Object& object);

    void compute_delete_prob(const std::vector<std::vector<int>>& sum, 
        const std::vector<std::vector<int>>& fre_del);
    
    // 检查标签区间块集合
    // zero_tag_partitions
    // one_tag_partitions
    // two_tag_partitions
    // three_tag_partitions
    // more_tag_partitions
    void check_tag_partition_sets() const;

    // 检查标签相关数据的一致性
    // disk_partition_usage_tagkind
    // disk_partition_usage_tagnum
    // disk_tag_kind
    // disk_tag_partition_num
    // tag_disk_partition
    void check_consistency() const;
    
    // 打印 disk_partition_usage_tagkind
    void printDiskPartitionUsageTagkind() const;

    // 打印 disk_tag_partition_num
    void printDiskTagPartitionNum() const;
};
