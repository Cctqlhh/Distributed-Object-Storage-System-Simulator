#pragma once
#include <vector>
#include "token_manager.h"

class Disk {
private:
    int id;
    int capacity;
    std::vector<int> storage;  // 存储单元，值为对象id
    int head_position;         // 磁头位置
    int max_tokens_;

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
    int refresh_token_manager();

    int jump(int position);
    int pass();
    int read();

};