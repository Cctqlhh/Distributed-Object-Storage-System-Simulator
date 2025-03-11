#include "disk.h"
#include <cassert>

Disk::Disk(int disk_id, int disk_capacity) 
    : id(disk_id)
    , capacity(disk_capacity)
    , head_position(1)
    , storage(disk_capacity + 1, 0) {}

bool Disk::write(int position, int object_id) {
    assert(position > 0 && position <= capacity);
    storage[position] = object_id;
    return true;
}

void Disk::erase(int position) {
    assert(position > 0 && position <= capacity);
    storage[position] = 0;
}

int Disk::get_head_position() const {
    return head_position;
}

bool Disk::is_free(int position) const {
    assert(position > 0 && position <= capacity);
    return storage[position] == 0;
}

int Disk::get_id() const {
    return id;
}

int Disk::get_capacity() const {
    return capacity;
}

int Disk::get_continuous_space() const {
    int max_continuous = 0;
    int current_continuous = 0;
    
    for (int i = 1; i <= capacity; i++) {
        if (!storage[i]) {  // 如果当前位置未被使用
            current_continuous++;
            max_continuous = std::max(max_continuous, current_continuous);
        } else {
            current_continuous = 0;
        }
    }
    
    return max_continuous;
}

std::pair<int, int> Disk::get_best_continuous_space() const {
    int max_continuous = 0;
    int best_start = 1;
    int current_continuous = 0;
    int current_start = 1;
    
    for (int i = 1; i <= capacity; i++) {
        if (!storage[i]) {
            if (current_continuous == 0) {
                current_start = i;
            }
            current_continuous++;
        } else {
            if (current_continuous > max_continuous) {
                max_continuous = current_continuous;
                best_start = current_start;
            }
            current_continuous = 0;
        }
    }
    
    if (current_continuous > max_continuous) {
        max_continuous = current_continuous;
        best_start = current_start;
    }
    
    return {max_continuous, best_start};
}

int Disk::get_used_count() const {
    int count = 0;
    for (int i = 1; i <= capacity; i++) {
        if (storage[i]) count++;
    }
    return count;
}