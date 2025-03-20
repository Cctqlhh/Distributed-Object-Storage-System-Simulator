#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <cmath> // 用于 ceil 计算
#include <algorithm>  // 用于排序
// #include "token_manager.h"
#include "object.h"
#include "request.h"
#include "disk.h"
#include "tag_manager.h"

#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define HEAT_THRESHOLD 25000  // 设定热度阈值
#define PARTITION_ALLOCATION_THRESHOLD 20  // 选取第20个时间片组进行分配

std::vector<Request> requests(MAX_REQUEST_NUM);
std::vector<Object> objects(MAX_OBJECT_NUM);
std::vector<Disk> disks;
// TokenManager* token_manager;

int T, M, N, V, G;
TagManager tagmanager(0, 0);

void preprocess() {
    // 时间片，标签个数，磁盘个数，磁盘容量，token数量
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    // 初始化全局 TagManager
    tagmanager = TagManager(M, N);
    // 计算时间片组数
    int slicing_count = (T - 1) / FRE_PER_SLICING + 1;
    // 数据接收
    // 存储时间片删除、写入、读取的数据
    std::vector<std::vector<int>> fre_del(M + 1, std::vector<int>(slicing_count + 1, 0));
    std::vector<std::vector<int>> fre_write(M + 1, std::vector<int>(slicing_count + 1, 0));
    std::vector<std::vector<int>> fre_read(M + 1, std::vector<int>(slicing_count + 1, 0));
    std::vector<std::vector<int>> sum(M + 1, std::vector<int>(slicing_count + 1, 0)); // 对象累积总大小
    std::vector<std::vector<int>> write_matrix(M + 1, std::vector<int>(slicing_count + 1, 0)); // 写入矩阵
    std::vector<std::vector<int>> conflict_matrix(M + 1, std::vector<int>(M + 1, 0)); // 冲突矩阵
    // fre_del
    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= slicing_count; j++)
        {
            scanf("%d", &fre_del[i][j]);
        }
    }
    // fre_write
    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= slicing_count; j++)
        {
            scanf("%d", &fre_write[i][j]);
        }
    }
    // fre_read
    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= slicing_count; j++)
        {
            scanf("%d", &fre_read[i][j]);
        }
    }

    // 数据处理
    // 计算 sum（对象累积总大小）
    for (int i = 1; i <= M; i++) {
        sum[i][1] = fre_write[i][1]; // 第一列直接赋值
        for (int j = 2; j <= slicing_count; j++) {
            sum[i][j] = sum[i][j - 1] + fre_write[i][j] - fre_del[i][j - 1];
            // 确保累积值不为负数
            if (sum[i][j] < 0) sum[i][j] = 0;
        }
    }

    // 计算写入矩阵（根据热度阈值）
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slicing_count; j++) {
            write_matrix[i][j] = (fre_write[i][j] >= HEAT_THRESHOLD) ? 1 : 0;
        }
    }

    // 计算冲突矩阵
    for (int a = 1; a <= M; a++) {
        for (int b = 1; b <= M; b++) {
            if (a == b) continue; // 不计算自身冲突
            int conflict_count = 0;
            for (int j = 1; j <= slicing_count; j++) {
                if (write_matrix[a][j] == 1 && write_matrix[b][j] == 1) {
                    conflict_count++;
                }
            }
            conflict_matrix[a][b] = conflict_count;
        }
    }

    // 初始化磁盘，把硬盘划分为二维区间块   有问题？？？
    disks.resize(N + 1);
    for (int i = 1; i <= N; i++) {
        disks[i] = Disk(i, V, G);
    }

    // 调用全局 tagmanager 进行标签分配
    tagmanager.allocate_tag_storage(sum, conflict_matrix, disks);

    // 预处理结束
    printf("OK\n");
    fflush(stdout);
}







void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);

    fflush(stdout);
}

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete); // 要删除的对象个数
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]); // 要删除的对象id
    }

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = objects[id].get_last_request();
        while (current_id != 0) { // 遍历请求链表
            if (!requests[current_id].is_completed()) {
                abort_num++; // 统计未完成的请求个数
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

void write_action()
{
    int n_write;
    scanf("%d", &n_write); // 要写的对象个数
    for (int i = 1; i <= n_write; i++) { // 要写的对象
        int id, size;
        scanf("%d%d%*d", &id, &size); // 对象id,对象大小,对象标签(未保存)
        objects[id] = Object(id, size);
        for (int j = 1; j <= REP_NUM; j++) { // 对象的第j个副本
        
            int disk_id = (id + j) % N + 1;
            if (!objects[id].write_replica(j, disks[disk_id])) {
                assert(false);  // 写入失败
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

void read_action()
{
// 获取读取请求
    int n_read;
    int request_id, object_id; 
    scanf("%d", &n_read); // 要读的请求个数，读取几个对象
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id); // 请求id,对象id
        requests[request_id] = Request(request_id, object_id); // 创建request对象
        requests[request_id].link_to_previous(objects[object_id].get_last_request()); // 请求链表链接
        objects[object_id].update_last_request(request_id); // 更新对象最后新增的请求 不影响读取，只方便删除
    }
    
//获取请求完成，开始读取操作
    static int current_request = 0;
    static int current_phase = 0;
    if (!current_request && n_read > 0) {
        current_request = request_id;
    }
    if (!current_request) { // 没有处理请求
        for (int i = 1; i <= N; i++) { // 遍历所有硬盘，输出# 不操作
            printf("#\n");
        }
        printf("0\n"); // 没有处理请求，读取完成0个请求
    } else {
        current_phase++; // 请求读取的对象id
        object_id = requests[current_request].get_object_id();
        for (int i = 1; i <= N; i++) {// 如果是对象的第1个副本存储的硬盘
            if (i == objects[object_id].get_replica_disk_id(1)) {
                if (current_phase % 2 == 1) { // 奇数，j跳转到对象块副本1的 第奇数块的存储单元
                    if (!disks[i].jump(current_phase / 2 + 1)) {  // 检查特定磁头的jump操作
                        printf("#\n");
                        continue;
                    }
                    printf("j %d\n", objects[object_id].get_storage_position(1, current_phase / 2 + 1));
                } else {
                    if (!disks[i].read()) {  // 检查特定磁头的read操作
                        printf("#\n");
                        continue;
                    }
                    printf("r#\n"); // 偶数，r，读取该块并指向下一个，#，结束操作
                }
            } else {
                printf("#\n"); // 不是对象的第1个副本存储的硬盘，输出#，不操作该硬盘
            }
        }
 // 读取完成 输出阶段
        if (current_phase == objects[object_id].get_size() * 2) {
            if (objects[object_id].is_deleted_status()) {
                printf("0\n"); // 对象已删除，读取完成也没用 所以0个请求读取成功
            } else {
                printf("1\n%d\n", current_request); // 对象未删除，读取完成1个请求读取成功
                requests[current_request].mark_as_completed();
            }
            current_request = 0;
            current_phase = 0;
        } else {
            printf("0\n"); // 读取未完成，输出0
        }
    }

    fflush(stdout);
}

int main()
{   
    preprocess();
    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        // token_manager->refresh();
        for (int i=1; i<=N; i++) {
            disks[i].refresh_token_manager();
        }
        timestamp_action();
        delete_action();
        write_action();
        read_action();
    }
    // delete token_manager;

    return 0;
}