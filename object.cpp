#include "object.h"
#include <cassert>

Object::Object(int id, int size) 
    : object_id(id)
    , size(size)
    , last_request_point(0)
    , is_deleted(false) {
    replica_disks.resize(REP_NUM + 1);  // REP_NUM + 1，保持与原代码一致从1开始索引
    unit_pos.resize(REP_NUM + 1);       // REP_NUM + 1
    for (int i = 1; i <= REP_NUM; i++) {
        unit_pos[i].resize(size + 1);
    }
}
// replica_idx 副本id
bool Object::write_replica(int replica_idx, Disk& disk) {
    assert(replica_idx > 0 && replica_idx <= REP_NUM);  // 添加副本索引检查
    int current_write_point = 0;
    int disk_capacity = disk.get_capacity();
    
    for (int i = 1; i <= disk_capacity; i++) {
        if (disk.is_free(i)) {
            if (disk.write(i, object_id)) {
                unit_pos[replica_idx][++current_write_point] = i;
                if (current_write_point == size) {
                    replica_disks[replica_idx] = disk.get_id();
                    return true;
                }
            }
        }
    }
    
    // 若写入失败是否应该保留已写入的部分内容
    assert(current_write_point == size);
    return false;
}

void Object::delete_replica(int replica_idx, Disk& disk) {
    for (int i = 1; i <= size; i++) {
        disk.erase(unit_pos[replica_idx][i]);
    }
}

int Object::get_storage_position(int replica_idx, int block_idx) const {
    assert(replica_idx > 0 && replica_idx < unit_pos.size());
    assert(block_idx > 0 && block_idx < unit_pos[replica_idx].size());
    return unit_pos[replica_idx][block_idx];
}

bool Object::is_deleted_status() const {
    return is_deleted;
}

void Object::mark_as_deleted() {
    is_deleted = true;
}

void Object::update_last_request(int request_id) {
    last_request_point = request_id;
}

int Object::get_last_request() const {
    return last_request_point;
}

int Object::get_size() const {
    return size;
}

int Object::get_replica_disk_id(int replica_idx) const {
    assert(replica_idx > 0 && replica_idx < replica_disks.size());
    return replica_disks[replica_idx];
}

// int Object::get_capacity() const {
//     return size;
// }

void Object::set_replica_disk(int replica_idx, int disk_id) {
    assert(replica_idx > 0 && replica_idx < replica_disks.size());
    replica_disks[replica_idx] = disk_id;
}

bool Object::is_valid_replica(int replica_idx) const {
    assert(replica_idx > 0 && replica_idx < replica_disks.size());
    return replica_disks[replica_idx] > 0;
}