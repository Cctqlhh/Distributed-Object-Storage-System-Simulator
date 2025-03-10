#pragma once
#include <vector>

class Disk {
private:
    int id;
    int capacity;
    std::vector<int> storage;  // 存储单元，值为对象id
    int head_position;         // 磁头位置

public:
    Disk() : id(0), capacity(0), head_position(1) {}  // 添加默认构造函数
    Disk(int id, int capacity);
    bool write(int position, int object_id);
    void erase(int position);
    int get_head_position() const;
    bool is_free(int position) const;
    int get_id() const;
    int get_capacity() const;

};