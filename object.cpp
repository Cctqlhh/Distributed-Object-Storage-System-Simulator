#include "object.h"


Object::Object(int id, int size, int tag_id) 
    : object_id(id)
    , size(size)
    , tag_id(tag_id)
    , last_request_point(0)
    , is_deleted(false)
    , replica_disks(REP_NUM + 1)
    , unit_pos(REP_NUM + 1, std::vector<int>(size + 1))
    , partition_id(REP_NUM + 1)
    , request_num(0)
    , is_continue(REP_NUM + 1) {}
    

// // 写策略1：采用散写策略
// bool Object::write_replica(int replica_idx, Disk& disk, int start_pos, int end_pos) {
//     assert(replica_idx > 0 && replica_idx <= REP_NUM);  // 添加副本索引检查 1-3
  
//     int current_write_point = 0;    // 当前写入大小
//     int disk_capacity = disk.get_capacity();
//     // 预分配空间，避免多次重新分配
//     std::vector<int> positions;
//     positions.reserve(size);
//     bool continuous = true;   // 默认连续写
    
//     // 先找到所有空闲位置，减少磁盘操作次数
//     for (int i = start_pos; i <= end_pos; i++) {
// //     for (int i = 1; i <= disk_capacity && current_write_point < size; i++) {
//         if (disk.is_free(i)) {
//             if (disk.write(i, object_id)) {
//                 unit_pos[replica_idx][++current_write_point] = i;
//                 // 写入完毕
//                 if (current_write_point == size) {
//                     replica_disks[replica_idx] = disk.get_id();
//                     // 减小区间块剩余大小 replica_idx 为1-3
//                     disk.reduce_residual_capacity(chosen_partitions[replica_idx - 1].second, size);
//                     is_continue[replica_idx] = continuous;
//                     return true;
//                 }
//             }
//         } else {
//             // 只要遇到非空闲位置，该副本就是散插
//             continuous = false; 
//         }
//     }
//     // 这里不可能写入失败，如果写入失败，直接报错
//     assert(current_write_point == size && "The write operation failed.");
//     return false;
// }

// 写策略2：先尝试连续写，再采用散写策略
bool Object::write_replica(int replica_idx, Disk& disk, int start_pos, int end_pos) {
    assert(replica_idx > 0 && replica_idx <= REP_NUM);  // 检查副本索引
    int disk_capacity = disk.get_capacity();

    bool continuous; // 是否连续写

    // 尝试连续写：扫描[start_pos, end_pos]寻找连续空闲区间块，长度等于size
    int contiguous_start = -1;
    int current_streak = 0;
    for (int i = start_pos; i <= end_pos; i++) {
        if (disk.is_free(i)) {
            if (current_streak == 0) {
                contiguous_start = i;  // 标记连续区间开始位置
            }
            current_streak++;
            if (current_streak == size) {
                break; // 找到足够长度的连续区间
            }
        } else {
            // 如果遇到非空闲位置，重置计数
            current_streak = 0;
            contiguous_start = -1;
        }
    }

    if (current_streak == size) {
        // 如果找到了连续区间，则连续写入
        continuous = true;
        for (int i = contiguous_start; i < contiguous_start + size; i++) {
            if (!disk.write(i, object_id)) {
                assert(false && "Contiguous write failed unexpectedly");
                return false;
            }
            // 记录写入的存储位置，注意这里下标从1开始
            unit_pos[replica_idx][i - contiguous_start + 1] = i;
        }
        replica_disks[replica_idx] = disk.get_id();
        disk.reduce_residual_capacity(chosen_partitions[replica_idx - 1].second, size);
        is_continue[replica_idx] = continuous; 
        return true;
    } else {
        // 如果没有找到连续空闲区域，则采用原来的散写策略
        continuous = false;
        int current_write_point = 0;
        for (int i = start_pos; i <= end_pos; i++) {
            if (disk.is_free(i)) {
                if (disk.write(i, object_id)) {
                    unit_pos[replica_idx][++current_write_point] = i;
                    if (current_write_point == size) {
                        replica_disks[replica_idx] = disk.get_id();
                        disk.reduce_residual_capacity(chosen_partitions[replica_idx - 1].second, size);
                        is_continue[replica_idx] = continuous; 
                        return true;
                    }
                }
            }
        }
        // 理论上不应该走到这里，写入失败直接断言
        assert(current_write_point == size && "The write operation failed.");
        return false;
    }
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

std::vector<std::pair<int, int>> Object::select_storage_partitions(
      TagManager& tag_manager,
      std::vector<Disk>& disks,
      const std::vector<std::vector<int>>& conflict_matrix) {

    chosen_partitions.clear();
    // std::unordered_set<int> used_disks;         // 已选过的硬盘，保证三个副本在不同硬盘上 
    std::set<int> used_disks;         // 已选过的硬盘，保证三个副本在不同硬盘上 

    // ========== Lambda：判断某个区间块是否满足写入条件 ==========
    auto can_use = [&](int disk_id, int part_id) {
        // 不允许重复硬盘   
        if (used_disks.count(disk_id)) return false;
        // 不允许空间不足    
        if (disks[disk_id].get_residual_capacity(part_id) < size) return false; 
        // 不允许该标签在该硬盘上的区间块数超出上限
        // int tag_count = 0;
        // for (int p = 1; p <= DISK_PARTITIONS; ++p) {
        //     if (tagmanager.disk_partition_usage_tagkind[disk_id][p].count(tag_id)) tag_count++;    // 该标签在该硬盘上的区间块数
        // }
        // // if (tag_count > MAX_PARTITIONS_PER_TAG) return false;
        // assert(tag_count == tagmanager.disk_tag_partition_num[disk_id][tag_id] && "tag_count != tagmanager.disk_tag_partition_num[disk_id][tag_id]");
        if (tagmanager.disk_tag_partition_num[disk_id][tag_id] > MAX_PARTITIONS_PER_TAG) return false;
        // 上述条件都满足，则该标签能够写入该硬盘上的该区间块
        return true;
    };

    // // ========== Lambda：在某个标签数量的区间集合中求出变量最大值 ==========
    // auto calculate_max = [&](std::set<std::pair<int, int>>& from_set, int expected_tag_count) {
    // };

    // ========== Lambda：从某个标签数量的区间集合中选出最优的区间块 ==========
    auto try_allocate_from_tag_partitions =
        [&](std::set<std::pair<int, int>>& from_set,
            std::set<std::pair<int, int>>* to_set,
            int expected_tag_count) {

            if ((int)chosen_partitions.size() == REP_NUM) return;  // 已经选够了，直接退出
            
            bool found = false;                                         // 是否找到合适的区间块
            std::pair<int, int> best_partition = {-1, -1};              // 最优区间块 {disk_id, part_id}
            int best_inner_conflict = -1;                               // 该对象标签与该区间块中标签的冲突值,越大越好
            int best_disk_conflict = std::numeric_limits<int>::max();   // 该对象标签与该硬盘中标签的冲突值,越小越好
            int best_disk_tag_partition_count = std::numeric_limits<int>::max(); // 该硬盘已分配标签的区间块数量,越小越好
            int best_residual = -1;                                     // 该区间块剩余容量,越大越好      
            double best_score = std::numeric_limits<double>::lowest();  // 最优区间块的得分,越大越好
            
            // 计算最大值
            int max_inner_conflict = std::numeric_limits<int>::min();   //最大内部冲突值
            int max_disk_conflict = std::numeric_limits<int>::min();    //最大整盘冲突值
            int max_count = std::numeric_limits<int>::min();            //最大硬盘上所有标签分区数量的和
            int max_residual = std::numeric_limits<int>::min();         //最大剩余容量
            for (const auto& candidate  : from_set) {
                int disk_id = candidate.first;
                int part_id = candidate.second;
                if (tag_manager.disk_partition_usage_tagkind[disk_id][part_id].count(tag_id)) continue;  // 该区间块包含该对象标签，跳过
                if (!can_use(disk_id, part_id)) continue;   // 该区间块不满足条件，跳过
                const auto& tags = tag_manager.disk_partition_usage_tagkind[disk_id][part_id];
                // if ((int)tags.size() != expected_tag_count) continue;   
                // 不是当前标签数量的区间块，跳过 
                if (expected_tag_count < 4) {
                    if ((int)tags.size() != expected_tag_count) continue;
                } else {
                    if ((int)tags.size() < expected_tag_count) continue;
                }

                // 计算区间内部冲突值：当前对象标签与该区间中所有标签的冲突值之和
                int inner_conflict = 0; 
                for (int t : tags) {
                    inner_conflict += conflict_matrix[tag_id][t]; // inner_conflict有可能还是0
                }
                if (inner_conflict > max_inner_conflict) max_inner_conflict = inner_conflict;

                // 计算整盘冲突值：当前对象标签与该磁盘上所有已分配标签的冲突值之和
                int disk_conflict = 0;
                for (int t : tag_manager.disk_tag_kind[disk_id]) {
                    disk_conflict += conflict_matrix[tag_id][t];
                }
                if (disk_conflict > max_disk_conflict) max_disk_conflict = disk_conflict;

                // 计算该硬盘上所有标签区间块数量的和（越小代表该磁盘负载越低），可能会超20，但不影响选择
                int disk_tag_partition_count = 0;
                for (const auto& entry : tag_manager.disk_tag_partition_num[disk_id]) {
                    disk_tag_partition_count += entry.second;
                }
                if (disk_tag_partition_count > max_count) max_count = disk_tag_partition_count;

                // 获取该区间块剩余容量
                int residual = disks[disk_id].get_residual_capacity(part_id);
                if (residual > max_residual) max_residual = residual;
            }

            // 遍历该集合下所有满足条件的区间块
            for (const auto& candidate  : from_set) {
                int disk_id = candidate.first;
                int part_id = candidate.second;
                if (tag_manager.disk_partition_usage_tagkind[disk_id][part_id].count(tag_id)) continue;  // 该区间块包含该对象标签，跳过
                if (!can_use(disk_id, part_id)) continue;   // 该区间块不满足条件，跳过
                const auto& tags = tag_manager.disk_partition_usage_tagkind[disk_id][part_id];
                // if ((int)tags.size() != expected_tag_count) continue;   
                // 不是当前标签数量的区间块，跳过 
                if (expected_tag_count < 4) {
                    if ((int)tags.size() != expected_tag_count) continue;
                } else {
                    if ((int)tags.size() < expected_tag_count) continue;
                }

                // 计算区间内部冲突值：当前对象标签与该区间中所有标签的冲突值之和
                int inner_conflict = 0; 
                for (int t : tags) {
                    inner_conflict += conflict_matrix[tag_id][t]; // inner_conflict有可能还是0
                }

                // 计算整盘冲突值：当前对象标签与该磁盘上所有已分配标签的冲突值之和
                int disk_conflict = 0;
                for (int t : tag_manager.disk_tag_kind[disk_id]) {
                    disk_conflict += conflict_matrix[tag_id][t];
                }

                // 计算该硬盘上所有标签分区数量的和（越小代表该磁盘负载越低），可能会超20，但不影响选择
                int disk_tag_partition_count = 0;
                for (const auto& entry : tag_manager.disk_tag_partition_num[disk_id]) {
                    disk_tag_partition_count += entry.second;
                }

                // 获取该区间块剩余容量
                int residual = disks[disk_id].get_residual_capacity(part_id);

                // // 为标签选择新区间块的策略1：
                // // 1. inner_conflict 越大越好
                // // 2. 若相同，则 disk_conflict 越小越好
                // // 3. 若仍相同，则 disk_tag_partition_count 越小越好
                // // 4. 最后选 residual 越大越好
                // if (!found ||
                //     (inner_conflict > best_inner_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_conflict == best_disk_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (inner_conflict == best_inner_conflict && disk_conflict == best_disk_conflict && disk_tag_partition_count == best_disk_tag_partition_count && residual > best_residual)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略2：
                // // 1. disk_tag_partition_count 越小越好
                // // 2. 若相同，则 inner_conflict 越大越好
                // // 3. 若相同，则 disk_conflict 越小越好
                // // 4. 若相同，则 residual 越大越好
                // if (!found ||
                //     (disk_tag_partition_count < best_disk_tag_partition_count) ||       
                //     (disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict > best_inner_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && disk_conflict == best_disk_conflict && residual > best_residual)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略3：
                // // 1. residual 越大越好 
                // // 2. 最后选 inner_conflict 越大越好
                // // 3. 若相同，则 disk_conflict 越小越好
                // // 4. 若仍相同，则 disk_tag_partition_count 越小越好
                // if (!found ||
                //     (residual > best_residual) ||
                //     (residual == best_residual && inner_conflict > best_inner_conflict) ||
                //     (residual == best_residual && inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict) ||
                //     (residual == best_residual && inner_conflict == best_inner_conflict && disk_conflict == best_disk_conflict && disk_tag_partition_count < best_disk_tag_partition_count)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略4：
                // // 1. disk_tag_partition_count 越小越好 
                // // 2. 若相同，则 residual 越大越好 
                // // 3. 若相同，则 inner_conflict 越大越好
                // // 4. 若相同，则 disk_conflict 越小越好
                // if (!found ||
                //     (disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && residual > best_residual) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && residual == best_residual && inner_conflict > best_inner_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && residual == best_residual && inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略5：
                // // 1. residual 越大越好 
                // // 2. 若相同，则 disk_tag_partition_count 越小越好
                // // 3. 若相同，则 inner_conflict 越大越好
                // // 4. 若相同，则 disk_conflict 越小越好
                // if (!found ||
                //     (residual > best_residual) ||
                //     (residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict > best_inner_conflict) ||
                //     (residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // 为标签选择新区间块的策略6：
                // 1. disk_tag_partition_count 越小越好 
                // 2. 若相同，则 residual 越大越好 
                // 3. 若相同，则 disk_conflict 越小越好
                // 4. 若相同，则 inner_conflict 越大越好
                if (!found ||
                    (disk_tag_partition_count < best_disk_tag_partition_count) ||
                    (disk_tag_partition_count == best_disk_tag_partition_count && residual > best_residual) ||
                    (disk_tag_partition_count == best_disk_tag_partition_count && residual == best_residual && disk_conflict < best_disk_conflict) ||
                    (disk_tag_partition_count == best_disk_tag_partition_count && residual == best_residual && disk_conflict == best_disk_conflict && inner_conflict > best_inner_conflict) 
                ) {
                    best_partition = candidate;
                    best_inner_conflict = inner_conflict;
                    best_disk_conflict = disk_conflict;
                    best_disk_tag_partition_count = disk_tag_partition_count;
                    best_residual = residual;
                    found = true;
                }

                // // 为标签选择新区间块的策略7：
                // // 策略 (I, D, R, P):
                // // 1. inner_conflict 越大越好
                // // 2. disk_conflict 越小越好
                // // 3. residual 越大越好
                // // 4. disk_tag_partition_count 越小越好
                // if (!found ||
                //     (inner_conflict > best_inner_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_conflict == best_disk_conflict && residual > best_residual) ||
                //     (inner_conflict == best_inner_conflict && disk_conflict == best_disk_conflict && residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略8：
                // // 策略 (I, P, D, R):
                // // 1. inner_conflict 越大越好
                // // 2. disk_tag_partition_count 越小越好
                // // 3. disk_conflict 越小越好
                // // 4. residual 越大越好
                // if (!found ||
                //     (inner_conflict > best_inner_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (inner_conflict == best_inner_conflict && disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict < best_disk_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict == best_disk_conflict && residual > best_residual)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略9：
                // // 策略 (I, P, R, D):
                // // 1. inner_conflict 越大越好
                // // 2. disk_tag_partition_count 越小越好
                // // 3. residual 越大越好
                // // 4. disk_conflict 越小越好
                // if (!found ||
                //     (inner_conflict > best_inner_conflict) ||
                //     (inner_conflict == best_inner_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (inner_conflict == best_inner_conflict && disk_tag_partition_count == best_disk_tag_partition_count && residual > best_residual) ||
                //     (inner_conflict == best_inner_conflict && disk_tag_partition_count == best_disk_tag_partition_count && residual == best_residual && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略10：
                // // 策略 (I, R, D, P):
                // // 1. inner_conflict 越大越好
                // // 2. residual 越大越好
                // // 3. disk_conflict 越小越好
                // // 4. disk_tag_partition_count 越小越好
                // if (!found ||
                //     (inner_conflict > best_inner_conflict) ||
                //     (inner_conflict == best_inner_conflict && residual > best_residual) ||
                //     (inner_conflict == best_inner_conflict && residual == best_residual && disk_conflict < best_disk_conflict) ||
                //     (inner_conflict == best_inner_conflict && residual == best_residual && disk_conflict == best_disk_conflict && disk_tag_partition_count < best_disk_tag_partition_count)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略11：
                // // 策略 (I, R, P, D):
                // // 1. inner_conflict 越大越好
                // // 2. residual 越大越好
                // // 3. disk_tag_partition_count 越小越好
                // // 4. disk_conflict 越小越好
                // if (!found ||
                //     (inner_conflict > best_inner_conflict) ||
                //     (inner_conflict == best_inner_conflict && residual > best_residual) ||
                //     (inner_conflict == best_inner_conflict && residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (inner_conflict == best_inner_conflict && residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略12：
                // // 策略 (D, I, P, R):
                // // 1. disk_conflict 越小越好
                // // 2. inner_conflict 越大越好
                // // 3. disk_tag_partition_count 越小越好
                // // 4. residual 越大越好
                // if (!found ||
                //     (disk_conflict < best_disk_conflict) ||
                //     (disk_conflict == best_disk_conflict && inner_conflict > best_inner_conflict) ||
                //     (disk_conflict == best_disk_conflict && inner_conflict == best_inner_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_conflict == best_disk_conflict && inner_conflict == best_inner_conflict && disk_tag_partition_count == best_disk_tag_partition_count && residual > best_residual)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略13：
                // // 策略 (D, I, R, P):
                // // 1. disk_conflict 越小越好
                // // 2. inner_conflict 越大越好
                // // 3. residual 越大越好
                // // 4. disk_tag_partition_count 越小越好
                // if (!found ||
                //     (disk_conflict < best_disk_conflict) ||
                //     (disk_conflict == best_disk_conflict && inner_conflict > best_inner_conflict) ||
                //     (disk_conflict == best_disk_conflict && inner_conflict == best_inner_conflict && residual > best_residual) ||
                //     (disk_conflict == best_disk_conflict && inner_conflict == best_inner_conflict && residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略14：
                // // 策略 (D, P, I, R):
                // // 1. disk_conflict 越小越好
                // // 2. disk_tag_partition_count 越小越好
                // // 3. inner_conflict 越大越好
                // // 4. residual 越大越好
                // if (!found ||
                //     (disk_conflict < best_disk_conflict) ||
                //     (disk_conflict == best_disk_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_conflict == best_disk_conflict && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict > best_inner_conflict) ||
                //     (disk_conflict == best_disk_conflict && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && residual > best_residual)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略15：
                // // 策略 (D, P, R, I):
                // // 1. disk_conflict 越小越好
                // // 2. disk_tag_partition_count 越小越好
                // // 3. residual 越大越好
                // // 4. inner_conflict 越大越好
                // if (!found ||
                //     (disk_conflict < best_disk_conflict) ||
                //     (disk_conflict == best_disk_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_conflict == best_disk_conflict && disk_tag_partition_count == best_disk_tag_partition_count && residual > best_residual) ||
                //     (disk_conflict == best_disk_conflict && disk_tag_partition_count == best_disk_tag_partition_count && residual == best_residual && inner_conflict > best_inner_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略16：
                // // 策略 (D, R, P, I):
                // // 1. disk_conflict 越小越好
                // // 2. residual 越大越好
                // // 3. disk_tag_partition_count 越小越好
                // // 4. inner_conflict 越大越好
                // if (!found ||
                //     (disk_conflict < best_disk_conflict) ||
                //     (disk_conflict == best_disk_conflict && residual > best_residual) ||
                //     (disk_conflict == best_disk_conflict && residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_conflict == best_disk_conflict && residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict > best_inner_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略17：
                // // 策略 (P, I, R, D):
                // // 1. disk_tag_partition_count 越小越好
                // // 2. inner_conflict 越大越好
                // // 3. residual 越大越好
                // // 4. disk_conflict 越小越好
                // if (!found ||
                //     (disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict > best_inner_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && residual > best_residual) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && residual == best_residual && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略18：
                // // 策略 (P, D, I, R):
                // // 1. disk_tag_partition_count 越小越好
                // // 2. disk_conflict 越小越好
                // // 3. inner_conflict 越大越好
                // // 4. residual 越大越好
                // if (!found ||
                //     (disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict < best_disk_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict == best_disk_conflict && inner_conflict > best_inner_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict == best_disk_conflict && inner_conflict == best_inner_conflict && residual > best_residual)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略19：
                // // 策略 (P, D, R, I):
                // // 1. disk_tag_partition_count 越小越好
                // // 2. disk_conflict 越小越好
                // // 3. residual 越大越好
                // // 4. inner_conflict 越大越好
                // if (!found ||
                //     (disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict < best_disk_conflict) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict == best_disk_conflict && residual > best_residual) ||
                //     (disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict == best_disk_conflict && residual == best_residual && inner_conflict > best_inner_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略20：
                // // 策略 (R, I, P, D):
                // // 1. residual 越大越好
                // // 2. inner_conflict 越大越好
                // // 3. disk_tag_partition_count 越小越好
                // // 4. disk_conflict 越小越好
                // if (!found ||
                //     (residual > best_residual) ||
                //     (residual == best_residual && inner_conflict > best_inner_conflict) ||
                //     (residual == best_residual && inner_conflict == best_inner_conflict && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (residual == best_residual && inner_conflict == best_inner_conflict && disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略21：
                // // 策略 (R, D, I, P):
                // // 1. residual 越大越好
                // // 2. disk_conflict 越小越好
                // // 3. inner_conflict 越大越好
                // // 4. disk_tag_partition_count 越小越好
                // if (!found ||
                //     (residual > best_residual) ||
                //     (residual == best_residual && disk_conflict < best_disk_conflict) ||
                //     (residual == best_residual && disk_conflict == best_disk_conflict && inner_conflict > best_inner_conflict) ||
                //     (residual == best_residual && disk_conflict == best_disk_conflict && inner_conflict == best_inner_conflict && disk_tag_partition_count < best_disk_tag_partition_count)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略22：
                // // 策略 (R, P, D, I):
                // // 1. residual 越大越好
                // // 2. disk_tag_partition_count 越小越好
                // // 3. disk_conflict 越小越好
                // // 4. inner_conflict 越大越好
                // if (!found ||
                //     (residual > best_residual) ||
                //     (residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict < best_disk_conflict) ||
                //     (residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && disk_conflict == best_disk_conflict && inner_conflict > best_inner_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略23：
                // // 策略 (D, R, I, P):
                // // 1. disk_conflict 越小越好
                // // 2. residual 越大越好
                // // 3. inner_conflict 越大越好
                // // 4. disk_tag_partition_count 越小越好
                // if (!found ||
                //     (disk_conflict < best_disk_conflict) ||
                //     (disk_conflict == best_disk_conflict && residual > best_residual) ||
                //     (disk_conflict == best_disk_conflict && residual == best_residual && inner_conflict > best_inner_conflict) ||
                //     (disk_conflict == best_disk_conflict && residual == best_residual && inner_conflict == best_inner_conflict && disk_tag_partition_count < best_disk_tag_partition_count)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // // 为标签选择新区间块的策略24：
                // // 为标签选择新区间块的策略24：
                // // 策略 (R, P, I, D):
                // // 1. residual 越大越好
                // // 2. disk_tag_partition_count 越小越好
                // // 3. inner_conflict 越大越好
                // // 4. disk_conflict 越小越好
                // if (!found ||
                //     (residual > best_residual) ||
                //     (residual == best_residual && disk_tag_partition_count < best_disk_tag_partition_count) ||
                //     (residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict > best_inner_conflict) ||
                //     (residual == best_residual && disk_tag_partition_count == best_disk_tag_partition_count && inner_conflict == best_inner_conflict && disk_conflict < best_disk_conflict)
                // ) {
                //     best_partition = candidate;
                //     best_inner_conflict = inner_conflict;
                //     best_disk_conflict = disk_conflict;
                //     best_disk_tag_partition_count = disk_tag_partition_count;
                //     best_residual = residual;
                //     found = true;
                // }

                // // 为标签选择新区间块的策略25：
                // // double score = -w1 * norm_count - w2 * norm_disk_conflict + w3 * norm_inner_conflict + w4 * norm_residual;
                // // 设置权重（可根据实际情况调优）
                // double w1 = 0.5; // 权重1：对硬盘已分配区间块数量的惩罚（越小越好）
                // double w2 = 0.1; // 权重2：对整盘冲突值的惩罚（越小越好）
                // double w3 = 0.1; // 权重3：对区间内部冲突值的奖励（越大越好）
                // double w4 = 0.5; // 权重4：对剩余容量的奖励（越大越好）
                // double norm_count = static_cast<double>(disk_tag_partition_count) / max_count;
                // double norm_inner_conflict = static_cast<double>(inner_conflict) / max_inner_conflict;
                // double norm_disk_conflict = static_cast<double>(disk_conflict) / max_disk_conflict;
                // double norm_residual = static_cast<double>(residual) / max_residual;
                // double score = -w1 * norm_count - w2 * norm_disk_conflict + w3 * norm_inner_conflict + w4 * norm_residual;
                // if (!found ||
                //     (score > best_score)
                // ) {
                //     best_partition = candidate;
                //     best_score = score;
                //     found = true;
                // }
            }

            // 找到了合适的区间块，更新所有信息
            if (found) {
                int disk_id = best_partition.first;                 // 硬盘ID
                int part_id = best_partition.second;                // 区间块ID
                chosen_partitions.emplace_back(disk_id, part_id);   // 只选择一个最优的区间块
                used_disks.insert(disk_id);
                if (expected_tag_count <= 3) {
                    from_set.erase(best_partition);                 // 从旧集合中删除
                }
                if (to_set) { 
                    to_set->insert(best_partition);                 // 加入新的集合（用于后续状态转移）
                }
            }
        };

    // ========== Step 1：优先使用已有分配给该标签的区间块 ==========
    for (const auto& [disk_id, parts] : tag_manager.tag_disk_partition[tag_id]) {
        for (int part_id : parts) {
            // 该硬盘未被选过，该硬盘该区间块有该标签，该区间块容量满足要求
            if (!used_disks.count(disk_id) &&                                                   
                tag_manager.disk_partition_usage_tagkind[disk_id][part_id].count(tag_id) &&    
                disks[disk_id].get_residual_capacity(part_id) >= size) {

                chosen_partitions.emplace_back(disk_id, part_id);
                used_disks.insert(disk_id);
                break;  // 该硬盘只能选择一个区间块，因此跳过后续区间块
            }
        }
        if ((int)chosen_partitions.size() == REP_NUM) break;
    }

    // while ((int)chosen_partitions.size() < REP_NUM) {
    // ========== Step 2：选择 0 标签区间块 ==========
    // 遍历 还需区间块数量 的次数，但每次遍历不一定有效
    for (int i = chosen_partitions.size() + 1; i <= REP_NUM; i++) {
        try_allocate_from_tag_partitions(tag_manager.zero_tag_partitions, &tag_manager.one_tag_partitions, 0);
    }
        // try_allocate_from_tag_partitions(tag_manager.zero_tag_partitions, &tag_manager.one_tag_partitions, 0);

    // ========== Step 3：选择 1 标签区间块 ==========
    // 遍历 还需区间块数量 的次数，但每次遍历不一定有效
    for (int i = chosen_partitions.size() + 1; i <= REP_NUM; i++) {
        try_allocate_from_tag_partitions(tag_manager.one_tag_partitions, &tag_manager.two_tag_partitions, 1);
    }
        // try_allocate_from_tag_partitions(tag_manager.one_tag_partitions, &tag_manager.two_tag_partitions, 1);

    // ========== Step 4：选择 2 标签区间块 ==========
    // 遍历 还需区间块数量 的次数，但每次遍历不一定有效
    for (int i = chosen_partitions.size() + 1; i <= REP_NUM; i++) {
        try_allocate_from_tag_partitions(tag_manager.two_tag_partitions, &tag_manager.three_tag_partitions, 2);
    }
        // try_allocate_from_tag_partitions(tag_manager.two_tag_partitions, &tag_manager.three_tag_partitions, 2);
        

    // ========== Step 5：选择 3 标签区间块 ==========
    // 遍历 还需区间块数量 的次数，但每次遍历不一定有效
    for (int i = chosen_partitions.size() + 1; i <= REP_NUM; i++) {
        try_allocate_from_tag_partitions(tag_manager.three_tag_partitions, &tag_manager.more_tag_partitions, 3); 
    }
        // try_allocate_from_tag_partitions(tag_manager.three_tag_partitions, &tag_manager.more_tag_partitions, 3); 

    // ========== Step 6（兜底）：遍历所有区间块，强行挑选满足条件的块 ==========
    // 遍历 还需区间块数量 的次数，但每次遍历不一定有效
    for (int i = chosen_partitions.size() + 1; i <= REP_NUM; i++) {
        try_allocate_from_tag_partitions(tag_manager.more_tag_partitions, nullptr, 4); 
    }
        // try_allocate_from_tag_partitions(tag_manager.more_tag_partitions, nullptr, 4); 
    // }

    // ========== 最终校验：必须分配到了三个副本 ==========
    assert(chosen_partitions.size() == REP_NUM && "Partition selection failed. Not enough valid partitions found.");
    return chosen_partitions;
}

std::vector<std::pair<int, int>> Object::get_chosen_partitions() const {
    return chosen_partitions;
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