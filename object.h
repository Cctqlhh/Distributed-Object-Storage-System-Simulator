#pragma once
#include "global.h"
// #include <vector>
// #include "disk.h"

#define REP_NUM 3

class Object {
private:
    int object_id;
    int size;
    std::vector<int> replica_disks;             // 副本所在磁盘ID
    std::vector<std::vector<int>> unit_pos;     // 每个副本的存储单元位置 3个副本，每个副本最多存储size个块
    std::vector<int> partition_id;              // 每个副本的存储单元所属的分区编号
    int last_request_point;                     // 最后一次读取该对象的 请求id
    bool is_deleted;
    int request_num;

public:
    Object(int id = 0, int size = 0);
    bool write_replica(int replica_idx, Disk& disk);
    void delete_replica(int replica_idx, Disk& disk);
    
    bool is_deleted_status() const; //判断对象是否已删除
    void mark_as_deleted(); //标记对象为已删除
    void update_last_request(int request_id); //更新最后一次读取该对象的请求id
    int get_last_request() const; //读取最后一次读取该对象的请求id
    int get_size() const; //读取对象大小
    void set_replica_disk(int replica_idx, int disk_id); //设置某个副本的磁盘id
    bool is_valid_replica(int replica_idx) const; // 判断replica_idx是否有效（没啥用）

    int get_storage_position(int replica_idx, int block_idx) const; //读取某个副本的某个块的存储位置
    int get_replica_disk_id(int replica_idx) const; //读取某个副本的磁盘id
    int get_partition_id(int replica_idx) const; //读取某个副本对应其硬盘的分区id

    int is_in_disk(int disk_id) const; //判断对象是否在某个磁盘上
    int get_which_unit(int disk_id, int unit_pos) const; //获取某个磁盘上某个存储单元存储的是对象的哪一块？
    
    void add_request();
    void reduce_request();
    int get_request_num() const;
};