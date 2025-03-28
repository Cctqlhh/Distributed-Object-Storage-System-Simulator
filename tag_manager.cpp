#include "tag_manager.h"

TagManager::TagManager(int M, int N, int slicing_count)
    : tag_delete_prob(M + 1, std::vector<double>(REP_NUM, 0.0f)), 
    disk_partition_usage_tagnum(N + 1, std::vector<std::vector<int>>(DISK_PARTITIONS + 1, std::vector<int>(M + 1, 0))), // 支持多个标签共存
    disk_partition_usage_tagkind(N + 1, std::vector<std::set<int>>(DISK_PARTITIONS + 1)),                   // 记录每个硬盘上的区间块已分配的标签种类数
    disk_tag_kind(N + 1),                  // 记录每个硬盘上的标签
    disk_tag_partition_num(N + 1),    // 记录每个硬盘上的标签及其区间块数量
    tag_disk_partition(M + 1)     // 记录每个标签分配的所有硬盘id和区间块id
    {}

void TagManager::init(const std::vector<std::vector<int>>& sum, const std::vector<std::vector<int>>& conflict_matrix, std::vector<Disk>& disks) {
    // 初始化局部变量
    std::vector<int> tag_required_blocks(M + 1, 0);                     // 每个标签需要的初始划分区间块数量
    std::vector<int> disk_remaining_partitions(N + 1, DISK_PARTITIONS); // 每个硬盘剩余区间块
    std::vector<bool> disk_used(N + 1, false);                          // 记录硬盘是否已被占用
    std::set<int> selected_disks;                             // 记录标签已选择的硬盘，最多3个
    std::vector<int> disk_first_empty_partition(N + 1, 1);              // 记录每个硬盘第一个为空的区间块id

    // 清空属性
    for (auto& inner_vec : disk_partition_usage_tagkind) {
        for (auto& s : inner_vec) {
            s.clear();
        }
    }
    for (auto& matrix : disk_partition_usage_tagnum) { // 已经初始化完毕，不需要再清空
        for (auto& vec : matrix) {
            std::fill(vec.begin(), vec.end(), 0);
        }
    }
    for (auto& s : disk_tag_kind) s.clear();                // 清空硬盘上的标签数量
    // 下面两个效果相同：对于 int 来说默认就是 0，因此，两种方法在后续访问时都能确保未存在的键对应的值是 0
    // for (auto& m : disk_tag_partition_num) m.clear();       // 清空硬盘上的标签及其区间块数量
    for (int disk_id = 1; disk_id <= N; disk_id++) {
        for (int tag = 1; tag <= M; tag++) {
            disk_tag_partition_num[disk_id][tag] = 0;
        }
    }
    // for (auto& n : tag_disk_partition) n.clear();           // 清空标签分配的所有硬盘id和区间块id
    for (int tag = 1; tag <= M; ++tag) {
        for (auto& pair : tag_disk_partition[tag]) {
            pair.second.clear();
        }
    }
    zero_tag_partitions.clear();
    one_tag_partitions.clear();
    two_tag_partitions.clear();
    three_tag_partitions.clear();
    more_tag_partitions.clear();

    // 因为一开始所有区间块含有的标签数量为0，所以将所有区间块添加到 zero_tag
    for (int disk_id = 1; disk_id <= N; ++disk_id) {
        for (int block = 1; block <= DISK_PARTITIONS; ++block) {
            zero_tag_partitions.insert({disk_id, block});
        }
    }

    // ========== Lambda：更新变量，维护属性 ==========
    // disk_id: 硬盘id，tag: 标签id
    auto update_tag_info_after_init = [&](int disk_id, int tag) {
        // 维护属性
        int allocated = 0;
        for (int partition_id = disk_first_empty_partition[disk_id]; partition_id <= DISK_PARTITIONS; partition_id++) {
            disk_partition_usage_tagkind[disk_id][partition_id].insert(tag);
            // tag_disk_partition[tag][disk_id].push_back(partition_id);
            tag_disk_partition[tag][disk_id].insert(partition_id);
            zero_tag_partitions.erase({disk_id, partition_id}); // 将该区间块从 zero_tag_partitions 中移除
            one_tag_partitions.insert({disk_id, partition_id}); // 将该区间块加入 one_tag_partitions
            allocated++;
            if (allocated == std::min(tag_required_blocks[tag], disk_remaining_partitions[disk_id])) break; // 分配足够的区间块后退出
        }
        disk_tag_kind[disk_id].insert(tag);
        // disk_tag_partition_num[disk_id][tag] += std::min(tag_required_blocks[tag], disk_remaining_partitions[disk_id]);
        // disk_tag_partition_num[disk_id][tag] = std::min(tag_required_blocks[tag], disk_remaining_partitions[disk_id]);
        // disk_tag_partition_num[disk_id][tag] = allocated;
        disk_tag_partition_num[disk_id][tag] += allocated;
        // 更新变量
        // disk_first_empty_partition[disk_id] += std::min(tag_required_blocks[tag], disk_remaining_partitions[disk_id]);
        disk_first_empty_partition[disk_id] += allocated;
        // disk_remaining_partitions[disk_id] = std::max(disk_remaining_partitions[disk_id] - tag_required_blocks[tag], 0);
        disk_remaining_partitions[disk_id] = disk_remaining_partitions[disk_id] - allocated;
        disk_used[disk_id] = true;          // 标记该硬盘已被占用
        selected_disks.insert(disk_id);     // 记录已选择的硬盘
    };

    // ========== Lambda：从未选择的硬盘中选择最佳硬盘 + 更新变量，维护属性 ==========
    // tag: 标签id，mode:选择剩余区间块是否足够的硬盘(0:足够，1:不足)
    auto select_best_disk = [&](int tag, int mode) {
        // std::vector<int> disk_conflict(N + 1, std::numeric_limits<int>::max());     // 记录硬盘冲突值
        // std::vector<int> disk_used_count(N + 1, std::numeric_limits<int>::max());   // 记录硬盘使用的区间块数量
        bool found = false;                                                         // 记录是否找到最佳硬盘
        int best_disk_id = -1;                                                      // 记录最佳硬盘id
        int best_conflict_sum = std::numeric_limits<int>::max();                    // 记录最佳硬盘冲突值
        int best_write_conflict_sum = std::numeric_limits<int>::max();              // 记录最佳硬盘写冲突值
        int best_used_count = std::numeric_limits<int>::max();                      // 记录最佳硬盘使用的区间块数量
        for (int disk_id = 1; disk_id <= N; disk_id++) {
            if (selected_disks.count(disk_id)) continue;
            if (disk_tag_partition_num[disk_id][tag] > MAX_PARTITIONS_PER_TAG) continue;        // 不允许该标签在该硬盘上的区间块数超出上限
            if (mode == 0) {
                if (disk_remaining_partitions[disk_id] < tag_required_blocks[tag]) continue;    // 剩余区间块不足
            }
            // 计算硬盘冲突值
            int conflict_sum = 0;
            for (const auto& exist_tag : disk_tag_kind[disk_id])
                conflict_sum += conflict_matrix[tag][exist_tag];
            // 计算硬盘已使用区间块数量
            int used_count = 0;
            for (const auto &entry : disk_tag_partition_num[disk_id])
                used_count += entry.second;
            // 计算硬盘写冲突值
            int write_conflict_sum = 0;
            for (const auto& exist_tag : disk_tag_kind[disk_id])
                write_conflict_sum += write_conflict_matrix[tag][exist_tag];

            // 选择策略1：
            // 1.选择硬盘冲突值最低的硬盘
            // 2.选择硬盘使用的区间块数量最小的硬盘
            if (!found || 
                (conflict_sum < best_conflict_sum) || 
                (conflict_sum == best_conflict_sum && used_count < best_used_count)) {
                best_conflict_sum = conflict_sum;
                best_used_count = used_count;
                best_disk_id = disk_id;
                found = true;
            }

            // // 选择策略2：
            // // 1.选择硬盘使用的区间块数量最小的硬盘
            // // 2.选择硬盘冲突值最低的硬盘
            // if (!found ||
            //     (used_count < best_used_count) || 
            //     (used_count = best_used_count && conflict_sum < best_conflict_sum)
            // ) {
            //     best_conflict_sum = conflict_sum;
            //     best_used_count = used_count;
            //     best_disk_id = disk_id;
            //     found = true;
            // }

            // // 选择策略3：
            // // 1.选择硬盘写冲突值最低的硬盘
            // // 2.选择硬盘使用的区间块数量最小的硬盘
            // if (!found ||
            //     (write_conflict_sum < best_write_conflict_sum) || 
            //     (write_conflict_sum = best_write_conflict_sum && used_count < best_used_count)
            // ) {
            //     best_write_conflict_sum = write_conflict_sum;
            //     best_used_count = used_count;
            //     best_disk_id = disk_id;
            //     found = true;
            // }

            // // 无脑添加第一个硬盘
            // if (found) {
            //     update_tag_info_after_init(best_disk_id, tag);
            //     if (selected_disks.size() == REP_NUM) return; // 已经选择了3个硬盘，返回
            // }
        }
        // 遍历完成后的最佳硬盘选择
        if (found) {
            update_tag_info_after_init(best_disk_id, tag);
            if (selected_disks.size() == REP_NUM) return; // 已经选择了3个硬盘，返回
        }
    };

    // 计算每个标签需要的最小区间块数
    for (int i = 1; i <= M; i++) { 
        int max_storage = SCALE * sum[i][std::min(PARTITION_ALLOCATION_THRESHOLD, (int)sum[i].size() - 1)];
        tag_required_blocks[i] = std::ceil((double)max_storage / disks[1].get_partition_size());
    }
    
    // 打印 tag_required_blocks
    std::cerr << "tag_required_blocks: " << std::endl;
    for (int i = 1; i <= M; i++) {
        std::cerr << "tag" << i << " : "<< tag_required_blocks[i] << std::endl;
    }

    // 遍历标签，为每个标签分配三个硬盘 
    std::vector<int> indices(M);
    for (int i = 0; i < M; ++i) {
        indices[i] = i + 1;
    }
    // 根据 tag_conflict_sum 降序排序索引
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return tag_conflict_sum[a] > tag_conflict_sum[b];
    });

    // // 打印排序后的indices值
    // std::cerr << "indices:";
    // for (int index : indices) {
    //     std::cerr << index << " ";
    // }
    // std::cerr << std::endl;

    // 优先给最冲突的标签分配硬盘,indices[i]为标签索引范围1~M
    for (int i = 0; i < M; i++) {
        selected_disks.clear(); // 记录该标签已选择的硬盘 

        // 第一选择该标签没有选过的硬盘，空的硬盘，剩余区间块足够的硬盘
        for (int disk_id = 1; disk_id <= N; disk_id++) {
            if (selected_disks.count(disk_id)) continue; // 不能重复选择硬盘
            if (disk_used[disk_id]) continue;
            if (disk_remaining_partitions[disk_id] < tag_required_blocks[indices[i]]) continue; // 剩余区间块不足
            update_tag_info_after_init(disk_id, indices[i]);
            if (selected_disks.size() == REP_NUM) break; // 已经选择了3个硬盘，退出循环
        }
        if (selected_disks.size() == REP_NUM) continue; // 已经选择了3个硬盘，退出循环

        // 第二选择该标签没有选过的硬盘，剩余区间块足够的硬盘
        for (int a = selected_disks.size() + 1; a <= REP_NUM; a++) {
            select_best_disk(indices[i], 0);
            if (selected_disks.size() == REP_NUM) break; // 已经选择了3个硬盘，退出循环
        }
        if (selected_disks.size() == REP_NUM) continue; // 已经选择了3个硬盘，退出循环

        // 第三选择该标签没有选过的硬盘，剩余区间块不足的硬盘
        for (int a = selected_disks.size() + 1; a <= REP_NUM; a++) {
            select_best_disk(indices[i], 1);
            if (selected_disks.size() == REP_NUM) break; // 已经选择了3个硬盘，退出循环
        }
        // 如果该标签还没有选择到3个硬盘，则报错
        assert(selected_disks.size() == REP_NUM && "Cannot allocate enough disks for this tag.(init)");
    }
}

void TagManager::update_tag_info_after_delete(const Object& object) {
    int tag = object.get_tag_id();
    std::vector<std::pair<int, int>> chosen_partitions = object.get_chosen_partitions();
    for (int j = 0; j < REP_NUM; j++) {
        int disk_id = chosen_partitions[j].first;
        int partition_id = chosen_partitions[j].second;
        disk_partition_usage_tagnum[disk_id][partition_id][tag] -= 1;  // 硬盘区间块的使用情况
        assert(disk_partition_usage_tagnum[disk_id][partition_id][tag] >= 0 && "The partition usage count is negative.");
        if (disk_partition_usage_tagnum[disk_id][partition_id][tag] == 0) {  // 该硬盘该区间块该标签的对象个数减到0
            disk_partition_usage_tagkind[disk_id][partition_id].erase(tag);  // 从该区间块的标签种类中移除该标签
            disk_tag_partition_num[disk_id][tag] -= 1;                      // 该硬盘该标签的区间块数量减1
            assert(disk_tag_partition_num[disk_id][tag] >= 0 && "The tag partition count is negative.");
            if (disk_tag_partition_num[disk_id][tag] == 0) {                // 该硬盘该标签的区间块数量减到0
                disk_tag_kind[disk_id].erase(tag);                          // 从该硬盘上移除该标签
            }
            // tag_disk_partition[tag][disk_id].erase(std::remove(tag_disk_partition[tag][disk_id].begin(), tag_disk_partition[tag][disk_id].end(), partition_id), 
            //                                         tag_disk_partition[tag][disk_id].end());  // 从该标签分配的硬盘id和区间块id中移除该区间块                   
            tag_disk_partition[tag][disk_id].erase(partition_id);
            int count = disk_partition_usage_tagkind[disk_id][partition_id].size();           // 在删除之后，该硬盘该区间块的标签数量
            if (count == 0) {          // 该区间块减到只有0个标签
                one_tag_partitions.erase({disk_id, partition_id});      // 将该区间块从 one_tag_partitions 中移除
                zero_tag_partitions.insert({disk_id, partition_id});    // 将该区间块加入 zero_tag_partitions
            } else if (count == 1) {  // 该区间块减到只有1个标签        
                two_tag_partitions.erase({disk_id, partition_id});      // 将该区间块从 two_tag_partitions 中移除
                one_tag_partitions.insert({disk_id, partition_id});     // 将该区间块加入 one_tag_partitions
            } else if (count == 2) {  // 该区间块减到只有2个标签
                three_tag_partitions.erase({disk_id, partition_id});    // 将该区间块从 three_tag_partitions 中移除
                two_tag_partitions.insert({disk_id, partition_id});     // 将该区间块加入 two_tag_partitions
            } else if (count == 3) {  // 该区间块减到只有3个标签
                more_tag_partitions.erase({disk_id, partition_id});     // 将该区间块从 more_tag_partitions 中移除
                three_tag_partitions.insert({disk_id, partition_id});   // 将该区间块加入 three_tag_partitions
            }
        }
    }
}

// // 原版本   有问题？？？
// void TagManager::update_tag_info_after_write(const Object& object) {
//     int tag = object.get_tag_id();
//     const auto& chosen_partitions = object.get_chosen_partitions();
//     for (const auto& [disk_id, part_id] : chosen_partitions) {
//         disk_partition_usage_tagnum[disk_id][part_id][tag] += 1;              
//         if (disk_partition_usage_tagnum[disk_id][part_id][tag] == 1) {
//             disk_partition_usage_tagkind[disk_id][part_id].insert(tag);  
//         }
//         disk_tag_kind[disk_id].insert(tag);
//         disk_tag_partition_num[disk_id][tag]++;
//         tag_disk_partition[tag][disk_id].push_back(part_id);
//     }
// }

// 现版本
void TagManager::update_tag_info_after_write(const Object& object) {
    int tag = object.get_tag_id();
    const auto& chosen_partitions = object.get_chosen_partitions();
    for (const auto& [disk_id, part_id] : chosen_partitions) {
        disk_partition_usage_tagnum[disk_id][part_id][tag] += 1;    // 每次都维护
        if (disk_partition_usage_tagkind[disk_id][part_id].count(tag) == 0) {   // 第一次使用该区间块
            disk_partition_usage_tagkind[disk_id][part_id].insert(tag);     // 插入该标签
            disk_tag_partition_num[disk_id][tag]++;                         // 该硬盘该标签的区间块数量加1
            // tag_disk_partition[tag][disk_id].push_back(part_id);            // 该标签分配的硬盘id和区间块id中插入该区间块
            tag_disk_partition[tag][disk_id].insert(part_id);              
        }
        if (disk_tag_kind[disk_id].count(tag) == 0) {   // 第一次使用该硬盘
            disk_tag_kind[disk_id].insert(tag);         // 该硬盘上插入该标签
        }
    }
}


void TagManager::compute_delete_prob(const std::vector<std::vector<int>>& sum, 
    const std::vector<std::vector<int>>& fre_del){
        for (int i = 1; i <= tag_delete_prob.size()-1; i++)
        {
            for (int j = 1; j <= tag_delete_prob[0].size()-1; j++)
            {
                tag_delete_prob[i][j] = sum[i][j] > 0 ? (double)fre_del[i][j] / sum[i][j] : 0;
            }
        }
    }


void TagManager::check_tag_partition_sets() const {
    // 检查 zero_tag_partitions：要求每个区间的标签数 == 0
    for (const auto &p : zero_tag_partitions) {
        int disk_id = p.first;
        int part_id = p.second;
        int count = disk_partition_usage_tagkind[disk_id][part_id].size();
        assert(count == 0 && "Zero tag partition check failed: Expected 0 tags.");
    }
    
    // 检查 one_tag_partitions：要求每个区间的标签数 == 1
    for (const auto &p : one_tag_partitions) {
        int disk_id = p.first;
        int part_id = p.second;
        int count = disk_partition_usage_tagkind[disk_id][part_id].size();
        assert(count == 1 && "One tag partition check failed: Expected 1 tag.");
    }
    
    // 检查 two_tag_partitions：要求每个区间的标签数 == 2
    for (const auto &p : two_tag_partitions) {
        int disk_id = p.first;
        int part_id = p.second;
        int count = disk_partition_usage_tagkind[disk_id][part_id].size();
        assert(count == 2 && "Two tag partition check failed: Expected 2 tags.");
    }
    
    // 检查 three_tag_partitions：要求每个区间的标签数 == 3
    for (const auto &p : three_tag_partitions) {
        int disk_id = p.first;
        int part_id = p.second;
        int count = disk_partition_usage_tagkind[disk_id][part_id].size();
        assert(count == 3 && "Three tag partition check failed: Expected 3 tags.");
    }
    
    // 检查 more_tag_partitions：要求每个区间的标签数 >= 4
    for (const auto &p : more_tag_partitions) {
        int disk_id = p.first;
        int part_id = p.second;
        int count = disk_partition_usage_tagkind[disk_id][part_id].size();
        assert(count >= 4 && "More tag partition check failed: Expected at least 4 tags.");
    }

     // 检查所有集合的总数必须等于 N * DISK_PARTITIONS
     int total = zero_tag_partitions.size() + one_tag_partitions.size() +
     two_tag_partitions.size() + three_tag_partitions.size() +
     more_tag_partitions.size();
    assert(total == (N * DISK_PARTITIONS) && "Total partition sets do not sum up to N * DISK_PARTITIONS.");

    // std::cerr << "All tag partition set checks passed." << std::endl;
}

void TagManager::check_consistency() const {
    // // 检查 1：对每个硬盘、每个分区，对每个标签，验证 disk_partition_usage_tagnum 与 disk_partition_usage_tagkind 是否一致
    // for (size_t disk_id = 1; disk_id < disk_partition_usage_tagkind.size(); ++disk_id) {
    //     for (int part_id = 1; part_id <= DISK_PARTITIONS; ++part_id) {
    //         for (int tag = 1; tag <= M; ++tag) {
    //             int count = disk_partition_usage_tagnum[disk_id][part_id][tag];
    //             bool inSet = (disk_partition_usage_tagkind[disk_id][part_id].count(tag) > 0);
    //             if (count > 0) {
    //                 assert(inSet && "Inconsistency: usage count > 0 but tag not found in tagkind set.");
    //             } else {
    //                 assert(!inSet && "Inconsistency: usage count == 0 but tag found in tagkind set.");
    //             }
    //         }
    //     }
    // }
    
    // 检查 2：对每个硬盘，验证 disk_tag_kind、disk_tag_partition_num 和 tag_disk_partition 的记录一致
    for (size_t disk_id = 1; disk_id < disk_tag_kind.size(); ++disk_id) {
        // 对于出现在 disk_tag_kind[disk_id] 中的每个标签 tag
        for (int tag : disk_tag_kind[disk_id]) {
            int computedCount = 0;
            // 统计该硬盘上有多少分区包含该标签
            for (int part_id = 1; part_id <= DISK_PARTITIONS; ++part_id) {
                if (disk_partition_usage_tagkind[disk_id][part_id].count(tag) > 0)
                    computedCount++;
            }
            
            // 检查 disk_tag_partition_num
            int recordedCount = 0;
            auto it = disk_tag_partition_num[disk_id].find(tag);
            if (it != disk_tag_partition_num[disk_id].end())
                recordedCount = it->second;
            assert(computedCount == recordedCount && "Inconsistency: computed partition count does not equal disk_tag_partition_num.");
            
            // 检查 tag_disk_partition
            int vectorCount = 0;
            auto it2 = tag_disk_partition[tag].find(disk_id);
            if (it2 != tag_disk_partition[tag].end())
                vectorCount = it2->second.size();
            assert(computedCount == vectorCount && "Inconsistency: computed partition count does not equal size of tag_disk_partition.");
        }
    }


    
    // std::cout << "All consistency checks passed." << std::endl;
}

void TagManager::printDiskPartitionUsageTagkind() const {
    std::cerr << "disk_partition_usage_tagkind:" << std::endl;
    for (int disk_id = 1; disk_id <= N; ++disk_id) {
        std::cerr << "Disk " << disk_id << ":" << std::endl;
        for (int partition = 1; partition <= DISK_PARTITIONS; ++partition) {
            std::cerr << "  Partition " << partition << ": ";
            for (const auto &tag : disk_partition_usage_tagkind[disk_id][partition]) {
                std::cerr << tag << " ";
            }
            std::cerr << std::endl;
        }
    }
}

void TagManager::printDiskTagPartitionNum() const {
    std::cerr << "disk_tag_partition_num:" << std::endl;
    // 从 1 开始（假设 disk_tag_partition_num[0] 未使用）
    for (size_t disk_id = 1; disk_id < disk_tag_partition_num.size(); ++disk_id) {
        std::cerr << "Disk " << disk_id << ":" << std::endl;
        // 遍历当前硬盘上的每个标签及其对应的区间块数量
        for (const auto& entry : disk_tag_partition_num[disk_id]) {
            std::cerr << "  Tag " << entry.first << ": " << entry.second << std::endl;
        }
    }
}
