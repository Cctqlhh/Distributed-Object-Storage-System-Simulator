#include "tag_manager.h"
#include <algorithm>
#include <limits>

TagManager::TagManager(int M, int N) {
    tag_disk_mapping.resize(M + 1, std::vector<int>(REP_NUM, -1));  // 初始化标签磁盘映射
    tag_partition_mapping.resize(M + 1, std::vector<int>(REP_NUM, 0));  // 初始化标签区间块映射
    disk_partition_usage.resize(N + 1, std::vector<int>(DISK_PARTITIONS, -1)); // -1 表示空闲
}

void TagManager::allocate_tag_storage(const std::vector<std::vector<int>>& sum, 
                                      const std::vector<std::vector<int>>& conflict_matrix,
                                      std::vector<Disk>& disks) {
    int M = sum.size() - 1;
    int N = disks.size() - 1;

    std::vector<int> tag_required_blocks(M + 1, 0);
    std::vector<int> disk_remaining_partitions(N + 1, DISK_PARTITIONS); // 每个磁盘剩余区间块
    std::vector<std::unordered_set<int>> disk_tags(N + 1); // 记录每个磁盘已有的标签

    // **1. 计算每个标签需要的最小区间块数**
    for (int i = 1; i <= M; i++) {
        int max_storage = sum[i][std::min(PARTITION_ALLOCATION_THRESHOLD, (int)sum[i].size() - 1)];
        tag_required_blocks[i] = std::ceil((double)max_storage / disks[1].get_partition_size());
    }

    // **2. 遍历标签，为每个标签分配三个磁盘**
    for (int tag = 1; tag <= M; tag++) {
        std::vector<std::pair<int, int>> candidate_disks; // 存储候选磁盘 {冲突值, 磁盘id}
        std::unordered_set<int> selected_disks; // 已选择的磁盘

        for (int j = 0; j < REP_NUM; j++) {  // 需要为该标签选择3个不同磁盘
            candidate_disks.clear();

            // 遍历所有磁盘，选择符合条件的磁盘
            for (int disk_id = 1; disk_id <= N; disk_id++) {
                if (selected_disks.count(disk_id)) continue; // 不能选已经选择过的磁盘
                if (disk_remaining_partitions[disk_id] < tag_required_blocks[tag]) continue; // 剩余区间块不足

                // 计算当前磁盘与该标签的冲突值
                int conflict_sum = 0;
                for (int existing_tag : disk_tags[disk_id]) {
                    conflict_sum += conflict_matrix[tag][existing_tag];
                }
                candidate_disks.push_back({conflict_sum, disk_id});
            }

            // 选择冲突值最小的磁盘
            if (!candidate_disks.empty()) {
                std::sort(candidate_disks.begin(), candidate_disks.end());
                int chosen_disk = candidate_disks[0].second;
                selected_disks.insert(chosen_disk);

                // 更新分配信息
                tag_disk_mapping[tag][j] = chosen_disk;
                tag_partition_mapping[tag][j] = tag_required_blocks[tag];
                disk_remaining_partitions[chosen_disk] -= tag_required_blocks[tag];
                disk_tags[chosen_disk].insert(tag);
            }
        }
    }
}

void TagManager::allocate_final_storage(std::vector<Disk>& disks) {
    int M = tag_disk_mapping.size() - 1;
    int N = disks.size() - 1;

    // **遍历所有标签，将它们的区间块分配到硬盘**
    for (int tag = 1; tag <= M; tag++) {
        for (int j = 0; j < REP_NUM; j++) {
            int disk_id = tag_disk_mapping[tag][j];
            int required_blocks = tag_partition_mapping[tag][j];

            if (disk_id == -1 || required_blocks == 0) continue; // 跳过未分配的标签

            // **寻找空闲区间块**
            int allocated_blocks = 0;
            for (int block = 0; block < DISK_PARTITIONS; block++) {
                if (disk_partition_usage[disk_id][block] == -1) { // 找到空闲区间块
                    disk_partition_usage[disk_id][block] = tag;
                    allocated_blocks++;
                }
                if (allocated_blocks == required_blocks) break; // 分配足够的区间块后退出
            }
        }
    }
}
