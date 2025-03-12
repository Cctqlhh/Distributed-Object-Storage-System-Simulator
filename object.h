#pragma once
#include <vector>
#include "disk.h"

#define REP_NUM 3

class Object {
private:
    int object_id;
    int size;
    std::vector<int> replica_disks;           // 副本所在磁盘ID
    std::vector<std::vector<int>> unit_pos;   // 每个副本的存储单元位置 3个副本，每个副本最多存储size个块
    int last_request_point;
    bool is_deleted;

public:
    Object(int id = 0, int size = 0);
    bool write_replica(int replica_idx, Disk& disk); // 把某副本写入某硬盘
    void delete_replica(int replica_idx, Disk& disk); // 删副本从某硬盘
    int get_storage_position(int replica_idx, int block_idx) const; // 获取某副本的存储位置
    bool is_deleted_status() const; // 检查对象是否已被删除
    void mark_as_deleted(); // 标记对象已被删除
    void update_last_request(int request_id); // 更新最后一次请求ID
    int get_last_request() const; // 获取最后一次请求ID
    int get_size() const; // 获取对象大小
    int get_replica_disk_id(int replica_idx) const; // 获取某副本所在的磁盘ID
    void set_replica_disk(int replica_idx, int disk_id); // 设置某副本所在的磁盘ID ？哪里调用？ 
    bool is_valid_replica(int replica_idx) const; // 副本是否存在硬盘里

    bool is_block_written(int replica_idx, int block_idx) const; // 添加一个辅助方法来判断副本的某个块是否已写入
    bool is_object_complete() const; // 添加一个方法检查对象是否完整（所有块至少有一个副本 已经写入），确保对象可完整读取
};