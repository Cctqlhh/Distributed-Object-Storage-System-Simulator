#include "tag_manager.h"

TagManager::TagManager(int M, int N, int slicing_count)
    : tag_delete_prob(M + 1, std::vector<double>(REP_NUM, 0.0f)), 
    init_tag_disk(M + 1, std::vector<int>(REP_NUM, -1)),  // 初始化标签硬盘映射
    init_tag_partition_num(M + 1, std::vector<int>(REP_NUM, 0)),     // 初始化标签区间块映射
    disk_partition_usage_tagnum(N + 1, std::vector<std::vector<int>>(DISK_PARTITIONS + 1, std::vector<int>(M, 0))), // 支持多个标签共存
    disk_partition_usage_tagkind(N + 1, std::vector<std::unordered_set<int>>(DISK_PARTITIONS + 1)),                   // 记录每个硬盘上的区间块已分配的标签种类数
    disk_tag_kind(N + 1),                  // 记录每个硬盘上的标签
    disk_tag_partition_num(N + 1),    // 记录每个硬盘上的标签及其区间块数量
    tag_disk_partition(M + 1)     // 记录每个标签分配的所有硬盘id和区间块id
    {}

void TagManager::calculate_tag_disk_requirement(const std::vector<std::vector<int>>& sum, 
                                      const std::vector<std::vector<int>>& conflict_matrix,
                                      std::vector<Disk>& disks) {
    std::vector<int> tag_required_blocks(M + 1, 0); // 每个标签需要的初始划分区间块数量
    std::vector<int> disk_remaining_partitions(N + 1, DISK_PARTITIONS); // 每个硬盘剩余区间块
    std::vector<std::unordered_set<int>> disk_tags(N + 1); // 记录每个硬盘已有的标签    ????????????????????????????????
    std::vector<bool> disk_used(N + 1, false); // 记录硬盘是否已被占用

    // 1. 计算每个标签需要的最小区间块数
    for (int i = 1; i <= M; i++) { 
        int max_storage = SCALE * sum[i][std::min(PARTITION_ALLOCATION_THRESHOLD, (int)sum[i].size() - 1)];
        tag_required_blocks[i] = std::ceil((double)max_storage / disks[1].get_partition_size());
    }
    
    // 2. 遍历标签，为每个标签分配三个硬盘 
    std::vector<int> indices(M); // 创建索引数组 [1, 2, ..., M]
    for (int i = 0; i < M; ++i) {
        indices[i] = i + 1;
    }
    // 根据 tag_conflict_sum 降序排序索引
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return tag_conflict_sum[a] > tag_conflict_sum[b]; // 降序
    });
    // 优先给最冲突的标签分配硬盘
    for (int i = 0; i < M; i++) {
        std::unordered_set<int> selected_disks;             // 记录已选择的硬盘 最多3个
        std::vector<std::pair<int, int>> candidate_disks;   // 候选 {冲突值, 硬盘 ID}
        // 需要为该标签选择3个不同硬盘
        for (int j = 0; j < REP_NUM; j++) {  
            candidate_disks.clear();

            // 优先选择空硬盘
            for (int disk_id = 1; disk_id <= N; disk_id++) {
                if (selected_disks.count(disk_id)) continue; // 不能重复选择硬盘
                if (disk_remaining_partitions[disk_id] < tag_required_blocks[indices[i]]) continue; // 剩余区间块不足
                if (!disk_used[disk_id]) {                      // 优先选完全空闲的硬盘
                    candidate_disks.push_back({0, disk_id});    // 空硬盘没有冲突值，即冲突值为0，直接插入
                    // selected_disks.insert(disk_id);             // 记录已选择的硬盘
                }
            }

            // 如果没有找到完全空闲的硬盘，再寻找低冲突硬盘
            if (candidate_disks.empty()) {
                for (int disk_id = 1; disk_id <= N; disk_id++) {
                    if (selected_disks.count(disk_id)) continue; // 不能重复选择硬盘
                    if (disk_remaining_partitions[disk_id] < tag_required_blocks[indices[i]]) continue; // 剩余区间块不足

                    // 计算当前硬盘的冲突值
                    int conflict_sum = 0;
                    for (int existing_tag : disk_tags[disk_id]) {
                        conflict_sum += conflict_matrix[indices[i]][existing_tag];
                    }
                    candidate_disks.push_back({conflict_sum, disk_id});
                }
            }

            // 如果 candidate_disks 仍然为空，所有硬盘没有满足条件的区间块，需要分配最大区间块数量的硬盘
            if (candidate_disks.empty()) {
                int largest_disk = -1;
                int max_space = 0;
                // 遍历所有硬盘
                for (int disk_id = 1; disk_id <= N; disk_id++) {
                    if (selected_disks.count(disk_id)) continue; // 不能重复选择硬盘
                    if (disk_remaining_partitions[disk_id] > max_space) {
                        max_space = disk_remaining_partitions[disk_id];
                        largest_disk = disk_id;
                    }
                }
                // 如果找到最大空闲空间，分配给该标签
                if (largest_disk != -1) {
                    init_tag_disk[indices[i]][j] = largest_disk;
                    init_tag_partition_num[indices[i]][j] = std::min(tag_required_blocks[indices[i]], max_space);
                    disk_remaining_partitions[largest_disk] -= init_tag_partition_num[indices[i]][j];
                    disk_tags[largest_disk].insert(indices[i]);
                    selected_disks.insert(largest_disk);
                    continue;
                }
            }

            // 如果 candidate_disks 不为空，有满足条件的区间块，需要从 candidate_disks 选择最佳硬盘
            if (!candidate_disks.empty()) {
                std::sort(candidate_disks.begin(), candidate_disks.end());
                int chosen_disk = candidate_disks[0].second;
                selected_disks.insert(chosen_disk);
                disk_used[chosen_disk] = true;

                // 更新分配信息
                init_tag_disk[indices[i]][j] = chosen_disk;
                init_tag_partition_num[indices[i]][j] = tag_required_blocks[indices[i]];
                disk_remaining_partitions[chosen_disk] -= tag_required_blocks[indices[i]];
                disk_tags[chosen_disk].insert(indices[i]);
            }
        }
    }

    // 后续可删
    // **3. 进行最终检测，每个标签必须有 3 个不同的硬盘**  
    for (int tag = 1; tag <= M; tag++) {
        std::unordered_set<int> unique_disks;
        
        for (int j = 0; j < REP_NUM; j++) {
            int disk_id = init_tag_disk[tag][j];
            assert(disk_id != -1 && "The disk label was not assigned correctly.");
            unique_disks.insert(disk_id);
        }

        assert(unique_disks.size() == REP_NUM && "Three copies of the label must be on different disks.");
    }
}


void TagManager::allocate_tag_disk_requirement(std::vector<Disk>& disks) {
    // 清空硬盘上的标签及其区间块数量
    for (auto& s : disk_tag_kind) s.clear();                // 清空硬盘上的标签数量
    for (auto& m : disk_tag_partition_num) m.clear();       // 清空硬盘上的标签及其区间块数量
    for (auto& n : tag_disk_partition) n.clear();           // 清空标签分配的所有硬盘id和区间块id

    // 初始化清空四个集合
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

    // 顺序遍历所有标签，将它们的区间块分配到硬盘
    for (int tag = 1; tag <= M; tag++) {
        for (int j = 0; j < REP_NUM; j++) {
            int disk_id = init_tag_disk[tag][j];
            int required_blocks = init_tag_partition_num[tag][j];
            
            if (disk_id == -1 || required_blocks == 0) continue; // 跳过未分配的标签
            
            disk_tag_kind[disk_id].insert(tag);  // 更新该硬盘上的标签
            disk_tag_partition_num[disk_id][tag] = required_blocks;  // 更新该硬盘上的标签及其区间块数量
            
            // 寻找空闲区间块
            int allocated_blocks = 0;
            for (int block = 1; block <= DISK_PARTITIONS; block++) {
                if (disk_partition_usage_tagkind[disk_id][block].empty()) {     // 找到空闲区间块
                    // 写入标签
                    disk_partition_usage_tagkind[disk_id][block].insert(tag);
                    tag_disk_partition[tag][disk_id].push_back(block);  // 记录该标签分配的硬盘id和区间块id，硬盘id从1开始，区间块id从1开始
                    zero_tag_partitions.erase({disk_id, block});        // 将该区间块从 zero_tag_partitions 中移除
                    one_tag_partitions.insert({disk_id, block});        // 将该区间块加入 one_tag_partitions
                    allocated_blocks++;
                    if (allocated_blocks == required_blocks) break; // 分配足够的区间块后退出
                }
            }
        }
    }
}

void TagManager::init(const std::vector<std::vector<int>>& sum, const std::vector<std::vector<int>>& conflict_matrix, std::vector<Disk>& disks) {
    
}

void TagManager::update_tag_info_after_init(const std::vector<Disk>& disks) {

}

void TagManager::update_tag_info_after_delete(const Object& object) {
    int tag = object.get_tag_id();
    std::vector<std::pair<int, int>> chosen_partitions = object.get_chosen_partitions();
    for (int j = 0; j < REP_NUM; j++) {
        int disk_id = chosen_partitions[j].first;
        int partition_id = chosen_partitions[j].second;
        disk_partition_usage_tagnum[disk_id][partition_id][tag] -= 1;  // 硬盘区间块的使用情况
        assert(disk_partition_usage_tagnum[disk_id][partition_id][tag] >= 0 && "The partition usage count is negative.");
        if (disk_partition_usage_tagnum[disk_id][partition_id][tag] == 0) {  // 该标签在该区间块的对象个数减到0
            disk_partition_usage_tagkind[disk_id][partition_id].erase(tag);  // 从该区间块的标签种类中移除该标签
            disk_tag_partition_num[disk_id][tag] -= 1;  // 该硬盘上的标签及其区间块数量
            if (disk_tag_partition_num[disk_id][tag] == 0) {    // 该硬盘上该标签的区间块数量减到0
                disk_tag_kind[disk_id].erase(tag);  // 从该硬盘上移除该标签
            }
            tag_disk_partition[tag][disk_id].erase(std::remove(tag_disk_partition[tag][disk_id].begin(), tag_disk_partition[tag][disk_id].end(), partition_id), 
                                                    tag_disk_partition[tag][disk_id].end());  // 从该标签分配的硬盘id和区间块id中移除该区间块                   
            int count = disk_partition_usage_tagkind[disk_id][partition_id].size(); // 在删除之后，该硬盘的该区间块的标签数量
            if (count == 0) {  // 该区间块减到只有0个标签
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

void TagManager::update_tag_info_after_write(const Object& object) {
    int tag = object.get_tag_id();
    const auto& chosen_partitions = object.get_chosen_partitions();
    for (const auto& [disk_id, part_id] : chosen_partitions) {
        disk_partition_usage_tagnum[disk_id][part_id][tag] += 1;
        if (disk_partition_usage_tagnum[disk_id][part_id][tag] == 1) {
            disk_partition_usage_tagkind[disk_id][part_id].insert(tag);
        }
        disk_tag_kind[disk_id].insert(tag);
        disk_tag_partition_num[disk_id][tag]++;
        tag_disk_partition[tag][disk_id].push_back(part_id);
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
