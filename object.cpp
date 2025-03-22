#include "object.h"
#include <cassert>

Object::Object(int id, int size, int tag_id) 
    : object_id(id)
    , size(size)
    , tag_id(tag_id)
    , last_request_point(0)
    , is_deleted(false)
    , replica_disks(REP_NUM + 1)
    , unit_pos(REP_NUM + 1, std::vector<int>(size + 1))
    , partition_id(REP_NUM + 1)
    , request_num(0) {}
    
// replica_idx 副本id
// bool Object::write_replica(int replica_idx, Disk& disk) {
//     assert(replica_idx > 0 && replica_idx <= REP_NUM);  // 添加副本索引检查
//     int current_write_point = 0;
//     int disk_capacity = disk.get_capacity();
    
//     for (int i = 1; i <= disk_capacity; i++) {
//         if (disk.is_free(i)) {
//             if (disk.write(i, object_id)) {
//                 unit_pos[replica_idx][++current_write_point] = i;
//                 if (current_write_point == size) {
//                     replica_disks[replica_idx] = disk.get_id();
//                     return true;
//                 }
//             }
//         }
//     }
    
//     // 若写入失败是否应该保留已写入的部分内容
//     assert(current_write_point == size);
//     return false;
// }
bool Object::write_replica(int replica_idx, Disk& disk) {
    assert(replica_idx > 0 && replica_idx <= REP_NUM);
    int current_write_point = 0;
    int disk_capacity = disk.get_capacity();
    
    // 预分配空间，避免多次重新分配
    std::vector<int> positions;
    positions.reserve(size);
    
    // 先找到所有空闲位置，减少磁盘操作次数
    for (int i = 1; i <= disk_capacity && current_write_point < size; i++) {
        if (disk.is_free(i)) {
            positions.push_back(i);
            current_write_point++;
        }
    }
    
    // 如果找到足够的空间，一次性写入
    if (current_write_point == size) {
        current_write_point = 0;
        for (int pos : positions) {
            disk.write(pos, object_id);
            unit_pos[replica_idx][++current_write_point] = pos;
        }
        replica_disks[replica_idx] = disk.get_id();
        return true;
    }
    
    assert(current_write_point == size);  // 保持与原版相同的行为
    return false;
}

void Object::delete_replica(int replica_idx, Disk& disk) {
    for (int i = 1; i <= size; i++) {
        disk.erase(unit_pos[replica_idx][i]);
    }
    request_num = 0;
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
    add_request();
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

int Object::get_partition_id(int replica_idx) const {
    return partition_id[replica_idx];
}

int Object::is_in_disk(int disk_id) const{
    for(int i = 1; i <= REP_NUM; i++){
        if(replica_disks[i] == disk_id) return i;
    }
    return 0;
}

int Object::get_which_unit(int disk_id, int unit_pos) const{
    int replica_idx = is_in_disk(disk_id);
    int pos = 0;
    for(int block_idx=1; block_idx<=size; block_idx++){
        pos = get_storage_position(replica_idx, block_idx);
        if(pos == unit_pos) return block_idx;
    }
    return 0;
}

void Object::add_request(){
    request_num++;
}

void Object::reduce_request(){
    request_num--;
}

int Object::get_request_num() const {
    return request_num;
}