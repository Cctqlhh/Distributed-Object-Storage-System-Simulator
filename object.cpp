#include "object.h"

Object::Object(int id, int size, int tag) 
    : object_id(id)
    , size(size)
    , tag_id(tag)  // **初始化标签**
    , last_request_point(0)
    , is_deleted(false)
    , replica_disks(REP_NUM + 1)
    , unit_pos(REP_NUM + 1, std::vector<int>(size + 1)){}

bool Object::write_replica(int replica_idx, Disk& disk, int start_pos, int end_pos) {
    assert(replica_idx > 0 && replica_idx <= REP_NUM);  // 添加副本索引检查 1-3

    // std::cerr<<"write_object: object_id="<<object_id
    //         <<",write_replica: replica_idx=" << replica_idx
    //         <<",disk_id="<<disk.get_id()
    //         <<",partition_id="<<chosen_partitions[replica_idx - 1].second
    //         <<",residual_capacity=" << disk.get_residual_capacity(chosen_partitions[replica_idx - 1].second)<<std::endl
    //         <<",size=" << size <<std::endl;

    int current_write_point = 0;    // 当前写入大小
    for (int i = start_pos; i <= end_pos; i++) {
        if (disk.is_free(i)) {
            if (disk.write(i, object_id)) {
                unit_pos[replica_idx][++current_write_point] = i;
                // 写入完毕
                if (current_write_point == size) {
                    replica_disks[replica_idx] = disk.get_id();
                    // 减小区间块剩余大小 replica_idx 为1-3
                    disk.reduce_residual_capacity(chosen_partitions[replica_idx - 1].second, size);
                    return true;
                }
            }
        }
    }
    // 这里不可能写入失败，如果写入失败，直接报错
    assert(current_write_point == size && "The write operation failed.");
    return false;
}

void Object::delete_replica(int replica_idx, Disk& disk) {
    assert(replica_idx > 0 && replica_idx <= REP_NUM);  // 添加副本索引检查 1-3

    // std::cerr<<"delete_object: object_id="<<object_id
    //         <<",delete_replica: replica_idx="<<replica_idx
    //         <<",disk_id="<<disk.get_id()
    //         <<",partition_id="<<chosen_partitions[replica_idx - 1].second
    //         <<",residual_capacity=" << disk.get_residual_capacity(chosen_partitions[replica_idx - 1].second)<<std::endl
    //         <<",size=" << size <<std::endl;

    // 自身存储数据不需要清零，只需要维护其他相关数据即可
    // 删除硬盘存储
    int current_delete_point = 0;    // 当前删除大小
    for (int i = 1; i <= size; i++) {
        disk.erase(unit_pos[replica_idx][i]);
        current_delete_point++;
    }
            
    // 增加区间块剩余大小   目前有问题？？？
    disk.increase_residual_capacity(chosen_partitions[replica_idx - 1].second, size);
    // 这里不可能删除失败，如果删除失败，直接报错
    assert(current_delete_point == size && "The delete operation failed.");
}

bool Object::write_object(std::vector<Disk>& disks) {
    int current_write_point;    // 当前写入大小
    // 把对象三个副本写入到三个区间块中
    for (int j = 1; j <= REP_NUM; j++) {
        current_write_point = 0;    // 当前写入大小
        int disk_id = chosen_partitions[j - 1].first;       // 硬盘ID
        int partition_id = chosen_partitions[j - 1].second; // 区间块ID
        const PartitionInfo& partition_info = disks[disk_id].get_partition_info(partition_id); // 区间块信息
        int start_pos = partition_info.start; // 区间块起始位置
        int end_pos = start_pos + partition_info.size - 1; // 区间块结束位置
        for (int i = start_pos; i <= end_pos; i++) {
            if (disks[disk_id].is_free(i)) {
                if (disks[disk_id].write(i, object_id)) {
                    unit_pos[j][++current_write_point] = i;
                    // 写入完毕
                    if (current_write_point == size) {
                        replica_disks[j] = disk_id;
                        // 减小区间块剩余大小
                        disks[disk_id].reduce_residual_capacity(chosen_partitions[j - 1].second, size);
                        return true;
                    }
                }
            }
        }
    }
    // 这里不可能写入失败，如果写入失败，直接报错
    assert(current_write_point == size && "The write operation failed.");
    return false;
}

void Object::delete_object(std::vector<Disk>& disks) {
    for (int j = 1; j <= REP_NUM; j++) {       
        int disk_id = replica_disks[j];    // 硬盘ID
        // 自身存储数据不需要清零，只需要维护其他相关数据即可
        // 删除硬盘存储
        int current_write_point = 0;    // 当前删除大小
        for (int i = 1; i <= size; i++) {
            disks[disk_id].erase(unit_pos[j][i]);
            current_write_point++;
        }
        // 增加区间块剩余大小
        disks[disk_id].increase_residual_capacity(chosen_partitions[j - 1].second, size);
        // 这里不可能删除失败，如果删除失败，直接报错
        assert(current_write_point == size && "The delete operation failed.");
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

int Object::get_tag_id() const {
    return tag_id;
}

std::vector<std::pair<int, int>> Object::select_storage_partitions(
    TagManager& tag_manager,
    std::vector<Disk>& disks,
    const std::vector<std::vector<int>>& conflict_matrix) {
    
    chosen_partitions.clear();
    std::unordered_set<int> used_disks;         // 已选过的硬盘，保证三个副本在不同硬盘上 
    int N = disks.size() - 1;                   // 硬盘总数

    // ========== Lambda：判断某个区间块是否满足写入条件 ==========
    auto can_use = [&](int disk_id, int part_id) {
        if (used_disks.count(disk_id)) return false;    // 不允许重复硬盘   
        if (disks[disk_id].get_residual_capacity(part_id) < size) return false; // 空间不足

        // 判断该标签在该硬盘上的区间块数是否已达到上限
        int tag_count = 0;
        for (int p = 1; p <= DISK_PARTITIONS; ++p) {
            if (tagmanager.disk_partition_usage_tagkind[disk_id][p].count(tag_id)) tag_count++;    // 该标签在该硬盘上的区间块数
        }
        if (tag_count > MAX_PARTITIONS_PER_TAG) return false;

        return true;
    };

    // ========== Lambda：从某个标签数量的区间集合中选出最优的区间块 ==========
    auto try_allocate_from_tag_partitions =
        [&](std::set<std::pair<int, int>>& from_set,
            std::set<std::pair<int, int>>* to_set,
            int expected_tag_count) {

            if ((int)chosen_partitions.size() == REP_NUM) return;  // 已经选够了，直接退出

            int max_inner_conflict = -1;
            int min_disk_conflict = std::numeric_limits<int>::max();
            std::pair<int, int> best_partition = {-1, -1};
            
            // 遍历该集合下所有满足条件的区间块
            for (const auto& [disk_id, part_id] : from_set) {
                if (!can_use(disk_id, part_id)) continue;
                const auto& tags = tag_manager.disk_partition_usage_tagkind[disk_id][part_id];
                if ((int)tags.size() != expected_tag_count) continue;

                // 计算当前区间块中标签与当前对象标签的冲突值
                int inner_conflict = 0; 
                for (int t : tags) inner_conflict += conflict_matrix[tag_id][t]; // inner_conflict有可能还是0

                // 计算整块磁盘中标签的冲突值
                int disk_conflict = 0;
                for (int t : tag_manager.disk_tag_kind[disk_id]) {
                    disk_conflict += conflict_matrix[tag_id][t];
                }

                // 策略：先选内部冲突大的，再在冲突值相同时选整盘冲突更小的
                if (
                    inner_conflict > max_inner_conflict ||
                    (inner_conflict == max_inner_conflict && disk_conflict < min_disk_conflict)
                ) {
                    max_inner_conflict = inner_conflict;
                    min_disk_conflict = disk_conflict;
                    best_partition = {disk_id, part_id};
                }
            }

            // 找到了合适的区间块，更新所有信息
            if (best_partition.first != -1) {
                int disk_id = best_partition.first, part_id = best_partition.second;
                chosen_partitions.emplace_back(disk_id, part_id);
                used_disks.insert(disk_id);
                
                
                // tag_manager.disk_partition_usage_tagnum[disk_id][part_id][tag_id] += 1; 
                // if (tag_manager.disk_partition_usage_tagnum[disk_id][part_id][tag_id] == 1) {
                //     tag_manager.disk_partition_usage_tagkind[disk_id][part_id].insert(tag_id);
                // }
                // tag_manager.disk_tag[disk_id].insert(tag_id);
                // tag_manager.disk_tag_partition_num[disk_id][tag_id]++;
                // tag_manager.tag_to_disk_partitions[tag_id][disk_id].push_back(part_id);

                from_set.erase(best_partition);                 // 从旧集合中删除  
                if (to_set) to_set->insert(best_partition);     // 加入新的集合（用于后续状态转移）
            }
        };

    // ========== Step 1：优先使用已有分配给该标签的区间块 ==========
    for (const auto& [disk_id, parts] : tag_manager.tag_disk_partition[tag_id]) {
        for (int part_id : parts) {
            if (!used_disks.count(disk_id) &&
                tag_manager.disk_partition_usage_tagkind[disk_id][part_id].count(tag_id) &&
                disks[disk_id].get_residual_capacity(part_id) >= size) {

                chosen_partitions.emplace_back(disk_id, part_id);
                used_disks.insert(disk_id);
                
                if ((int)chosen_partitions.size() == REP_NUM) break;
            }
        }
        if ((int)chosen_partitions.size() == REP_NUM) break;
    }

    // ========== Step 2：选择 0 标签区间块 ==========
    try_allocate_from_tag_partitions(
        tag_manager.zero_tag_partitions,
        &tag_manager.one_tag_partitions,
        0);

    // ========== Step 3：选择 1 标签区间块 ==========
    try_allocate_from_tag_partitions(
        tag_manager.one_tag_partitions,
        &tag_manager.two_tag_partitions,
        1);

    // ========== Step 4：选择 2 标签区间块 ==========
    try_allocate_from_tag_partitions(
        tag_manager.two_tag_partitions,
        &tag_manager.three_tag_partitions,
        2);

    // ========== Step 5：选择 3 标签区间块 ==========
    try_allocate_from_tag_partitions(
        tag_manager.three_tag_partitions,
        nullptr,
        3);

    // ========== Step 6（兜底）：遍历所有区间块，强行挑选满足条件的块 ==========
    if ((int)chosen_partitions.size() < REP_NUM) {
        for (int disk_id = 1; disk_id <= N && (int)chosen_partitions.size() < REP_NUM; ++disk_id) {
            if (used_disks.count(disk_id)) continue;
            for (int part_id = 1; part_id <= DISK_PARTITIONS; ++part_id) {
                if (can_use(disk_id, part_id)) {
                    chosen_partitions.emplace_back(disk_id, part_id);
                    used_disks.insert(disk_id);
                    break;
                    // // 同样更新 TagManager
                    // tag_manager.disk_partition_usage_tagnum[disk_id][part_id][tag_id] += 1;
                    // if (tag_manager.disk_partition_usage_tagnum[disk_id][part_id][tag_id] == 1) {
                    //     tag_manager.disk_partition_usage_tagkind[disk_id][part_id].insert(tag_id);
                    // }
                    // tag_manager.disk_tag[disk_id].insert(tag_id);
                    // tag_manager.disk_tag_partition_num[disk_id][tag_id]++;
                    // tag_manager.tag_to_disk_partitions[tag_id][disk_id].push_back(part_id);
                    
                }
            }
        }
    }

    // ========== 最终校验：必须分配到了三个副本 ==========
    assert(chosen_partitions.size() == REP_NUM && "Partition selection failed. Not enough valid partitions found.");
    return chosen_partitions;
}

std::vector<std::pair<int, int>> Object::get_chosen_partitions() const {
    return chosen_partitions;
}