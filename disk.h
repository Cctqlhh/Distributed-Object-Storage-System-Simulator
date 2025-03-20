#pragma once
#include <vector>
#include "token_manager.h"

#define DISK_PARTITIONS 20  // 定义硬盘分区数

class Disk {
private:
    int id;
    int capacity;
    std::vector<int> storage;  // 存储单元，值为对象id
    int head_position;         // 磁头位置
    int max_tokens_;
    int partition_size;        // 每个分区的大小
    std::vector<int> partition_info;  // 存储每个位置对应的分区


public:
    TokenManager* token_manager;
    Disk() : id(0), capacity(0), head_position(1), max_tokens_(0) {}  // 添加默认构造函数
    Disk(int id, int capacity, int max_tokens);
    bool write(int position, int object_id);
    void erase(int position);
    int get_head_position() const;
    bool is_free(int position) const;
    int get_id() const;
    int get_capacity() const;

    int get_distance_to_head(int position) const; // 返回目标位置到磁头的距离（存储单元数（对象块数））
    int get_need_token_to_head(int position) const; // 返回目标位置到磁头的距离（token数
    void refresh_token_manager();

    int jump(int position);
    int pass();
    int read();
    
    int get_partition(int position) const; // 计算存储单元 `position` 所属的分区
    int get_partition_size() const; // 获取磁盘的 `partition_size`


};