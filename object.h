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
    int last_request_point;                  // 最后一次读取该对象的 请求id
    bool is_deleted;

public:
    Object(int id = 0, int size = 0);
    bool write_replica(int replica_idx, Disk& disk);
    void delete_replica(int replica_idx, Disk& disk);
    int get_storage_position(int replica_idx, int block_idx) const;
    bool is_deleted_status() const;
    void mark_as_deleted();
    void update_last_request(int request_id);
    int get_last_request() const;
    int get_size() const;
    int get_replica_disk_id(int replica_idx) const;
    void set_replica_disk(int replica_idx, int disk_id);
    bool is_valid_replica(int replica_idx) const;
};