#include <cstdio>
#include <vector>
#include "storage_manager.h"

int main() {
    int T, M, N, V, G;
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    
    std::vector<Request> requests(MAX_REQUEST_NUM);
    std::vector<Object> objects(MAX_OBJECT_NUM);
    std::vector<Disk> disks(N + 1);
    std::vector<TagStats> tag_stats(M + 1);

    for (int i = 1; i <= N; i++) {
        disks[i] = Disk(i, V);
    }

    StorageManager storage_manager(requests, objects, disks, tag_stats, T, M, N, V, G);
    storage_manager.init_system();

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        storage_manager.refresh_tokens();
        storage_manager.process_timestamp(t);
        storage_manager.process_delete();
        storage_manager.process_write();
        storage_manager.process_read();
    }

    return 0;
}