// #pragma once
// #include <vector>
// #include <unordered_set>
// #include "disk.h"
// #include "tag_manager.h"
// #include <cassert>
// #include <set>
// #include <algorithm>

// #define REP_NUM 3

// class TagManager;

#pragma once
#include "global.h"

class Object {
private:
    int object_id;
    int size;
    // 对象写入时更新
    std::vector<int> replica_disks;             // 副本所在磁盘ID 
    // 对象写入时更新
    std::vector<std::vector<int>> unit_pos;     // 每个副本的存储单元位置 3个副本，每个副本最多存储size个块
    int last_request_point;                     // 最后一次读取该对象的 请求id
    bool is_deleted;
    int tag_id;                                 // 标签属性
    // 该对象选择的三个区间块   {硬盘id，区间块id}
    std::vector<std::pair<int, int>> chosen_partitions;

public:
    Object(int id = 0, int size = 0, int tag = 0);
    // 写入一个副本，修改自身存储数据，硬盘存储数据，区间块大小减少。
    bool write_replica(int replica_idx, Disk& disk, int start_pos, int end_pos);
    // 删除一个副本，修改硬盘存储数据，区间块大小增加。
    void delete_replica(int replica_idx, Disk& disk);
    // 在硬盘上写入该对象的三个副本
    bool write_object(std::vector<Disk>& disk);
    // 在硬盘上删除该对象的三个副本
    void delete_object(std::vector<Disk>& disks);
    // 获取该对象在硬盘上的存储位置
    int get_storage_position(int replica_idx, int block_idx) const;
    bool is_deleted_status() const;
    void mark_as_deleted();
    void update_last_request(int request_id);
    int get_last_request() const;
    int get_size() const;
    int get_replica_disk_id(int replica_idx) const;
    void set_replica_disk(int replica_idx, int disk_id);
    bool is_valid_replica(int replica_idx) const;
    int get_tag_id() const;  // 获取对象的标签 ID

    // 根据对象标签进行对象写入区间块的选择（确保所有存储规则）
    std::vector<std::pair<int, int>> select_storage_partitions(
        TagManager& tag_manager,
        std::vector<Disk>& disks,
        const std::vector<std::vector<int>>& conflict_matrix);
    
    std::vector<std::pair<int, int>> get_chosen_partitions() const;
};