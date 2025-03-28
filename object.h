#pragma once
#include "global.h"

// class TagManager;

#pragma once
#include "global.h"

class Object {
private:
    int object_id;
    int size;
    int tag_id;                                 // 标签属性
    // 对象写入时更新
    std::vector<int> replica_disks;             // 副本所在磁盘ID 
    // 对象写入时更新
    std::vector<std::vector<int>> unit_pos;     // 每个副本的存储单元位置 3个副本，每个副本最多存储size个块
    int last_request_point;                     // 最后一次读取该对象的 请求id
    bool is_deleted;
    // 该对象选择的三个区间块   {硬盘id，区间块id}
    std::vector<std::pair<int, int>> chosen_partitions;
    std::vector<int> partition_id;              // 每个副本的存储单元所属的分区编号
    int request_num;
    std::vector<int> active_requests;           // 存储当前对象正在处理的请求id
    std::vector<bool> is_continue;              // 对象的连续性标记 1~3

public:
    Object(int id = 0, int size = 0, int tag_id = 0);
    // 写入一个副本，修改自身存储数据，硬盘存储数据，区间块大小减少。
//     bool write_replica(int replica_idx, Disk& disk);
    bool write_replica(int replica_idx, Disk& disk, int start_pos, int end_pos);
    // 删除一个副本，修改硬盘存储数据，区间块大小增加。
    void delete_replica(int replica_idx, Disk& disk);
    // 获取该对象在硬盘上的存储位置
    bool is_deleted_status() const; //判断对象是否已删除
    void mark_as_deleted(); //标记对象为已删除
  
    void update_last_request(int request_id); //更新最后一次读取该对象的请求id
    int get_last_request() const; //读取最后一次读取该对象的请求id
    int get_size() const; //读取对象大小
    int get_tag_id() const{
        return tag_id;
    } //读取对象的标签id
  
  
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

    void add_active_request(int request_id) {
        active_requests.push_back(request_id);
    }

    std::vector<int>& get_active_requests() {
        return active_requests;
    }

    void remove_completed_request(int request_id) {
        auto it = std::find(active_requests.begin(), active_requests.end(), request_id);
        if (it != active_requests.end()) {
            active_requests.erase(it);
        }
    }

    // 根据对象标签进行对象写入区间块的选择（确保所有存储规则）
    std::vector<std::pair<int, int>> select_storage_partitions(TagManager& tag_manager,
                                                                std::vector<Disk>& disks,
                                                                const std::vector<std::vector<int>>& conflict_matrix);
    std::vector<std::pair<int, int>> get_chosen_partitions() const;

};