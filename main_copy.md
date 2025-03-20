#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <cmath> // 用于 ceil 计算
// #include "token_manager.h"

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define HEAT_THRESHOLD 25000  // 设定热度阈值

typedef struct Request_ {
    int object_id;
    int prev_id;
    bool is_done;
} Request;

typedef struct Object_ {
    int replica[REP_NUM + 1];
    int* unit[REP_NUM + 1];
    int size;
    int last_request_point;
    bool is_delete;
} Object;

Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];

int T, M, N, V, G;
int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
int disk_point[MAX_DISK_NUM];

void preprocess() {
    // 时间片，标签个数，磁盘个数，磁盘容量，token数量
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
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

    // // 初始化磁盘
    // disks.resize(N + 1);
    // for (int i = 1; i <= N; i++) {
    //     disks[i] = Disk(i, V, G);
    // }

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

void do_object_delete(const int* object_unit, int* disk_unit, int size)
{
    for (int i = 1; i <= size; i++) {
        disk_unit[object_unit[i]] = 0;
    }
}

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                abort_num++;
            }
            current_id = request[current_id].prev_id;
        }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                printf("%d\n", current_id);
            }
            current_id = request[current_id].prev_id;
        }
        for (int j = 1; j <= REP_NUM; j++) {
            do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size);
        }
        object[id].is_delete = true;
    }

    fflush(stdout);
}

void do_object_write(int* object_unit, int* disk_unit, int size, int object_id)
{
    int current_write_point = 0;
    for (int i = 1; i <= V; i++) {
        if (disk_unit[i] == 0) {
            disk_unit[i] = object_id;
            object_unit[++current_write_point] = i;
            if (current_write_point == size) {
                break;
            }
        }
    }

    assert(current_write_point == size);
}

void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size;
        scanf("%d%d%*d", &id, &size);
        object[id].last_request_point = 0;
        for (int j = 1; j <= REP_NUM; j++) {
            object[id].replica[j] = (id + j) % N + 1;
            object[id].unit[j] = static_cast<int*>(malloc(sizeof(int) * (size + 1)));
            object[id].size = size;
            object[id].is_delete = false;
            do_object_write(object[id].unit[j], disk[object[id].replica[j]], size, id);
        }

        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", object[id].replica[j]);
            for (int k = 1; k <= size; k++) {
                printf(" %d", object[id].unit[j][k]);
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
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
    }

    static int current_request = 0;
    static int current_phase = 0;
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
        object_id = request[current_request].object_id;
        for (int i = 1; i <= N; i++) {
            if (i == object[object_id].replica[1]) {
                if (current_phase % 2 == 1) {
                    printf("j %d\n", object[object_id].unit[1][current_phase / 2 + 1]);
                } else {
                    printf("r#\n");
                }
            } else {
                printf("#\n");
            }
        }

        if (current_phase == object[object_id].size * 2) {
            if (object[object_id].is_delete) {
                printf("0\n");
            } else {
                printf("1\n%d\n", current_request);
                request[current_request].is_done = true;
            }
            current_request = 0;
            current_phase = 0;
        } else {
            printf("0\n");
        }
    }

    fflush(stdout);
}

void clean()
{
    for (auto& obj : object) {
        for (int i = 1; i <= REP_NUM; i++) {
            if (obj.unit[i] == nullptr)
                continue;
            free(obj.unit[i]);
            obj.unit[i] = nullptr;
        }
    }
}

int main()
{
    preprocess();

    for (int i = 1; i <= N; i++) {
        disk_point[i] = 1;
    }

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        timestamp_action();
        delete_action();
        write_action();
        read_action();
    }
    clean();

    return 0;
}