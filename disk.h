#pragma once
#include <vector>

class Disk {
private:
    int id;
    int capacity;
    std::vector<int> storage;  // 存储单元，值为对象id
    int head_position;         // 磁头位置
    
    // 添加空闲块快速查找
    // std::vector<int> free_positions;

public:
    Disk() : id(0), capacity(0), head_position(1) {}  // 添加默认构造函数
    Disk(int id, int capacity);
    bool write(int position, int object_id);
    void erase(int position);
    int get_head_position() const;
    bool is_free(int position) const;
    int get_id() const;
    int get_capacity() const;

        // 快速找到空闲位置
    // int find_next_free_position() {
    //     if (!free_positions.empty()) {
    //         int pos = free_positions.back();
    //         free_positions.pop_back();
    //         return pos;
    //     }
    //     return -1;
    // }
};