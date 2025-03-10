#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include "token_manager.h"
#include "object.h"
#include "request.h"
#include "disk.h"

// #define MAX_DISK_NUM (10 + 1)
// #define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
// #define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
 
// typedef struct Request_ {
//     int object_id;
//     int prev_id;
//     bool is_done;
// } Request;

// typedef struct Object_ {
//     int replica[REP_NUM + 1];
//     int* unit[REP_NUM + 1];
//     int size;
//     int last_request_point;
//     bool is_delete;
// } Object;

// Request request[MAX_REQUEST_NUM];
// Object object[MAX_OBJECT_NUM];
std::vector<Request> requests(MAX_REQUEST_NUM);
std::vector<Object> objects(MAX_OBJECT_NUM);
std::vector<Disk> disks;
TokenManager* token_manager;

int T, M, N, V, G;
// int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
// int disk_point[MAX_DISK_NUM];

void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);

    fflush(stdout);
}

// void do_object_delete(const int* object_unit, int* disk_unit, int size)
// {
//     for (int i = 1; i <= size; i++) {
//         disk_unit[object_unit[i]] = 0;
//     }
// }

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
        // int current_id = object[id].last_request_point;
        int current_id = objects[id].get_last_request();
        while (current_id != 0) { // 遍历请求链表
            // if (request[current_id].is_done == false) {
            if (!requests[current_id].is_completed()) {
                abort_num++; // 统计未完成的请求个数
            }
            // current_id = request[current_id].prev_id;            
            current_id = requests[current_id].get_prev_id();
        }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        // int current_id = object[id].last_request_point;
        int current_id = objects[id].get_last_request();
        while (current_id != 0) {
            // if (request[current_id].is_done == false) {
            if (!requests[current_id].is_completed()) {
                printf("%d\n", current_id);
            }
            // current_id = request[current_id].prev_id;
            current_id = requests[current_id].get_prev_id();
        }
        for (int j = 1; j <= REP_NUM; j++) {
            // do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size);
            int disk_id = objects[id].get_replica_disk_id(j);
            objects[id].delete_replica(j, disks[disk_id]);
        }
        // object[id].is_delete = true;
        objects[id].mark_as_deleted();
    }

    fflush(stdout);
}

// void do_object_write(int* object_unit, int* disk_unit, int size, int object_id)
// {
//     int current_write_point = 0;
//     for (int i = 1; i <= V; i++) {
//         if (disk_unit[i] == 0) { // 找到硬盘空闲块
//             disk_unit[i] = object_id; // 记录硬盘空闲块保存的对象id
//             object_unit[++current_write_point] = i; // 记录当前对象块保存的存储单元id
//             if (current_write_point == size) { // 对象块全部存储完成 break
//                 break;
//             }
//         }
//     }

//     assert(current_write_point == size); // 断言对象块全部存储完成，否则报错
// }

void write_action()
{
    int n_write;
    scanf("%d", &n_write); // 要写的对象个数
    for (int i = 1; i <= n_write; i++) { // 要写的对象
        int id, size;
        scanf("%d%d%*d", &id, &size); // 对象id,对象大小,对象标签(未保存)
        // object[id].last_request_point = 0; // 初始化请求链表为空
        objects[id] = Object(id, size);
        for (int j = 1; j <= REP_NUM; j++) { // 对象的第j个副本
        
            int disk_id = (id + j) % N + 1;
            if (!objects[id].write_replica(j, disks[disk_id])) {
                assert(false);  // 写入失败
            }

            // object[id].replica[j] = (id + j) % N + 1; // 计算第j个副本分配到硬盘编号(简单分散)
            // object[id].unit[j] = static_cast<int*>(malloc(sizeof(int) * (size + 1))); // 计算第j个副本的对象块
            // object[id].size = size;
            // object[id].is_delete = false;
            // do_object_write(object[id].unit[j], disk[object[id].replica[j]], size, id);
        }

        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            // printf("%d", object[id].replica[j]);
            printf("%d", objects[id].get_replica_disk_id(j));
            for (int k = 1; k <= size; k++) {
                // printf(" %d", object[id].unit[j][k]);
                printf(" %d", objects[id].get_storage_position(j, k));
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

void read_action()
{
    int n_read;
    int request_id, object_id; 
    scanf("%d", &n_read); // 要读的请求个数，读取几个对象
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id); // 请求id,对象id
        // request[request_id].object_id = object_id; // 记录i请求信息，目标对象id
        // request[request_id].prev_id = object[object_id].last_request_point; // 记录i请求信息，上一个请求id
        // object[object_id].last_request_point = request_id; // 更新对象最后一次读取的请求id
        // request[request_id].is_done = false;
        requests[request_id] = Request(request_id, object_id);
        requests[request_id].link_to_previous(objects[object_id].get_last_request());
        objects[object_id].update_last_request(request_id);
    }

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
        current_phase++;
        // object_id = request[current_request].object_id; // 请求读取的对象id
        object_id = requests[current_request].get_object_id();
        for (int i = 1; i <= N; i++) {
            // if (i == object[object_id].replica[1]) { // 如果是对象的第1个副本存储的硬盘
            if (i == objects[object_id].get_replica_disk_id(1)) {
                if (current_phase % 2 == 1) { // 奇数，j跳转到对象块副本1的 第奇数块的存储单元
                    if (!token_manager->consume_jump(i)) {  // 检查特定磁头的jump操作
                        printf("#\n");
                        continue;
                    }
                    // printf("j %d\n", object[object_id].unit[1][current_phase / 2 + 1]);
                    printf("j %d\n", objects[object_id].get_storage_position(1, current_phase / 2 + 1));
                } else {
                    if (!token_manager->consume_read(i)) {  // 检查特定磁头的read操作
                        printf("#\n");
                        continue;
                    }
                    printf("r#\n"); // 偶数，r，读取该块并指向下一个，#，结束操作
                }
            } else {
                printf("#\n"); // 不是对象的第1个副本存储的硬盘，输出#，不操作该硬盘
            }
        }

        // if (current_phase == object[object_id].size * 2) { // 读取完成
        if (current_phase == objects[object_id].get_size() * 2) {
            // if (object[object_id].is_delete) { 
            if (objects[object_id].is_deleted_status()) {
                printf("0\n"); // 对象已删除，读取完成也没用 所以0个请求读取成功
            } else {
                printf("1\n%d\n", current_request); // 对象未删除，读取完成1个请求读取成功
                // request[current_request].is_done = true;
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

// void clean()
// {
//     for (auto& obj : object) {
//         for (int i = 1; i <= REP_NUM; i++) {
//             if (obj.unit[i] == nullptr)
//                 continue;
//             free(obj.unit[i]);
//             obj.unit[i] = nullptr;
//         }
//     }
// }

int main()
{
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    
    // 初始化磁盘
    disks.resize(N + 1);
    for (int i = 1; i <= N; i++) {
        // disks.emplace_back(i, V);
        disks[i] = Disk(i, V);
    }

    token_manager = new TokenManager(G, N);
    // 统计 同一标签对象信息，多组时间片 删写读 规律
    // del 对象标签i,第j组时间片(1800一组),总共删除的对象大小(多少对象块)
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    // write
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    // read
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    printf("OK\n");
    fflush(stdout);

    // // 初始化硬盘指针
    // for (int i = 1; i <= N; i++) {
    //     disk_point[i] = 1;
    // }

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        token_manager->refresh();
        timestamp_action();
        delete_action();
        write_action();
        read_action();
    }
    delete token_manager;
    // clean();

    return 0;
}