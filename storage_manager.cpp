#include "storage_manager.h"
#include <cstdio>
#include <cassert>

int StorageManager::current_request = 0;
int StorageManager::current_phase = 0;

StorageManager::StorageManager(std::vector<Request>& reqs, std::vector<Object>& objs,
                             std::vector<Disk>& dsks, std::vector<TagStats>& stats,
                             int t, int m, int n, int v, int g)
    : requests(reqs), objects(objs), disks(dsks), tag_stats(stats),
      T(t), M(m), N(n), V(v), G(g) {
    token_manager = new TokenManager(G, N);
}

StorageManager::~StorageManager() {
    delete token_manager;
}

void StorageManager::init_system() {
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int freq;
            scanf("%d", &freq);
            tag_stats[i].del_freq += freq;
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int freq;
            scanf("%d", &freq);
            tag_stats[i].write_freq += freq;
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int freq;
            scanf("%d", &freq);
            tag_stats[i].read_freq += freq;
        }
        tag_stats[i].calculate_score(T);
    }

    printf("OK\n");
    fflush(stdout);
}

int StorageManager::find_optimal_disk(int size, int tag) const {
    if (tag_stats[tag].hot_score < 0.5) {
        return -1;  // 使用默认分配策略
    }

    int best_disk = 1;
    double best_score = -1;
    
    for (int i = 1; i <= N; i++) {
        const auto& disk = disks[i];        
        auto [continuous_space, start_pos] = disk.get_best_continuous_space();
        if (continuous_space < size) continue;
        
        double space_score = continuous_space / (1.0 * V);
        double load_score = 1.0 - (disk.get_used_count() / (1.0 * V));
        double disk_score = space_score * 0.7 + load_score * 0.3;
        
        if (disk_score > best_score) {
            best_score = disk_score;
            best_disk = i;
        }
    }
    return best_disk;
}

void StorageManager::process_timestamp(int timestamp) {
    int input_timestamp;
    scanf("%*s%d", &input_timestamp);  // 读取实际的时间戳输入
    printf("TIMESTAMP %d\n", input_timestamp);
    fflush(stdout);
}

void StorageManager::process_delete() {
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = objects[id].get_last_request();
        while (current_id != 0) {
            if (!requests[current_id].is_completed()) {
                abort_num++;
            }     
            current_id = requests[current_id].get_prev_id();
        }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = objects[id].get_last_request();
        while (current_id != 0) {
            if (!requests[current_id].is_completed()) {
                printf("%d\n", current_id);
            }
            current_id = requests[current_id].get_prev_id();
        }
        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id = objects[id].get_replica_disk_id(j);
            objects[id].delete_replica(j, disks[disk_id]);
        }
        objects[id].mark_as_deleted();
    }
    fflush(stdout);
}

void StorageManager::process_write() {
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag);
        objects[id] = Object(id, size);

        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id;
            int optimal_disk = find_optimal_disk(size, tag);
            if (optimal_disk != -1) {
                disk_id = optimal_disk;
            } else {
                disk_id = (id + j) % N + 1;
            }
            
            if (!objects[id].write_replica(j, disks[disk_id])) {
                assert(false);
            }
        }

        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", objects[id].get_replica_disk_id(j));
            for (int k = 1; k <= size; k++) {
                printf(" %d", objects[id].get_storage_position(j, k));
            }
            printf("\n");
        }
    }
    fflush(stdout);
}

void StorageManager::process_read() {
    int n_read;
    int request_id, object_id; 
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        requests[request_id] = Request(request_id, object_id);
        requests[request_id].link_to_previous(objects[object_id].get_last_request());
        objects[object_id].update_last_request(request_id);
    }

    if (!current_request && n_read > 0) {
        current_request = request_id;
    }
    
    if (!current_request) {
        for (int i = 1; i <= N; i++) {
            printf("#\n");
        }
        printf("0\n");
    } else {
        current_phase++;
        object_id = requests[current_request].get_object_id();
        handle_disk_operations(object_id);
    }
    fflush(stdout);
}

void StorageManager::handle_disk_operations(int object_id) {
    int target_disk = objects[object_id].get_replica_disk_id(1);

    for (int i = 1; i <= N; i++) {
        if (i != target_disk) {
            printf("#\n");
            continue;
        }
        bool is_jump = (current_phase % 2 == 1);
        if (is_jump) {
            if (!token_manager->consume_jump(i)) {
                printf("#\n");
                continue;
            }
            printf("j %d\n", objects[object_id].get_storage_position(1, current_phase / 2 + 1));
        } else {
            if (!token_manager->consume_read(i)) {
                printf("#\n");
                continue;
            }
            printf("r#\n");
        }

    }

    bool is_operation_complete = (current_phase == objects[object_id].get_size() * 2);

    if (is_operation_complete) {
        if (objects[object_id].is_deleted_status()) {
            printf("0\n");
        } else {
            printf("1\n%d\n", current_request);
            requests[current_request].mark_as_completed();
        }
        current_request = 0;
        current_phase = 0;
    } else {
        printf("0\n");
    }
}

void StorageManager::refresh_tokens() {
    token_manager->refresh();
}