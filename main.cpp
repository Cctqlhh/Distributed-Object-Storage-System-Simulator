#include "global.h"

std::vector<Request> requests(MAX_REQUEST_NUM);
std::vector<Object> objects(MAX_OBJECT_NUM);
std::vector<Disk> disks;
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
    // std::cerr << "TIMESTAMP " << timestamp << std::endl;
    fflush(stdout);
}

void delete_action(int t)
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

void write_action(int t)
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

void read_action(int t)
{
// 获取读取请求
    int n_read;
    int request_id, object_id; 
    std::unordered_set<int> finish_requests;
    scanf("%d", &n_read); // 要读的请求个数，读取几个对象
    // 添加新请求
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id); // 请求id,对象id
        requests[request_id] = Request(request_id, object_id, t, objects[object_id].get_size()); // 创建request对象
        requests[request_id].link_to_previous(objects[object_id].get_last_request()); // 请求链表链接
        objects[object_id].update_last_request(request_id); // 更新对象最后新增的请求 不影响读取，只方便删除

    }

    // 每个磁头寻找要读的对象进行读操作  
    // bool stop = false;
    for(int i=1; i<=N;){
        // 维护已有请求 应当磁头空闲时候进行维护
        // std::cerr << "id " << i << " head_is_free " << disks[i].head_is_free() << std::endl;
        // std::cerr << "last_ok  " << disks[i].last_ok << std::endl;
        if(disks[i].head_is_free()){ //磁头空闲，需要设置新的读取对象
            disks[i].reflash_partition_score(); // 刷新分数,以便重新写入
            bool has_request = false;
            // for(auto & req: requests){ // 遍历读取请求
            for (auto it = requests.begin() + 1; it != requests.end(); ++it) {
                auto & req = *it;
                int object_id = req.get_object_id();
                if(object_id == 0) break; // 目标不存在，则停止，请求遍历完了
                int replica_idx = objects[object_id].is_in_disk(i); // 获取副本id
                if(replica_idx == 0) continue; //副本id为0,目标不在当前硬盘上，则看下一个
                
                // std::cerr << "id " << i << " object_id " << object_id << std::endl;
                // 目标存在，且在该硬盘上有副本，更新硬盘上该目标请求的信息

                // int partition_id = objects[object_id].get_partition_id(replica_idx); // 获取副本所在分区
                int partition_id = disks[i].get_partition_id(objects[object_id].get_storage_position(replica_idx, 1)); // 获取副本所在分区

                float score = req.compute_time_score_update(t); 
                score *= req.get_size_score();
                // std::cerr << "id " << i << " score " << score << std::endl;
                if(!has_request && score > 0) has_request = true;
                // std::cerr << "disk_id  " << i << "  has_request  " << has_request << std::endl;
                // std::cerr << "partition_id  " << partition_id << std::endl;
                disks[i].update_partition_info(partition_id, score); // 更新分区得分,同时更新
            }
            // 若无请求?
            // std::cerr << "id " << i << " has_request " << has_request << std::endl;
            if(!has_request) {
                printf("#\n"); // 当前硬盘无请求，不操作
                i++;
                continue; // 下一个硬盘
            }
            // 若有请求需要处理
            disks[i].set_head_busy();
        }
        // std::cerr << "id " << i << " head_is_free " << disks[i].head_is_free() << std::endl;
        // 磁头忙（设置好了要读取的对象）进行读取操作
        int head = disks[i].get_head_position();
        // auto part_p = disks[i].get_top_partition();
        const PartitionInfo* part_p;
        if(!disks[i].last_ok) { // 上次未读取完
            // std::cerr << "last not ok" << std::endl;
            part_p = disks[i].part_p; // 继续上次区间
        }else{ // 上次读完了
            part_p = disks[i].get_pop_partition(); // 获取最高分区
            if(part_p == disks[i].part_p){ // 还是原来分区
                auto second_part_p = disks[i].get_pop_partition(); // 获取第二高分区
                if(disks[i].part_p->next == second_part_p && second_part_p->score > 0){ // 如果是下一个分区，先操作下一个分区(下一个分区有分数的话)
                    disks[i].part_p = second_part_p;
                    part_p = second_part_p;
                }else{
                    disks[i].part_p = part_p; // 更新当前分区
                }
            }else{
                disks[i].part_p = part_p; // 更新当前分区
            }
        }
                
        if(part_p->score <= 0){ // 最好
            // std::cerr << "id " << i << " part_p->score " << part_p->score << std::endl;
            // i++;
            // printf("#\n"); // 当前硬盘无请求，不操作
            // continue;
        }
        
        // 取出存储了对象的位置vector
        std::vector<int> storage_ = disks[i].get_storage();
        std::vector<int> subStorage(storage_.begin() + part_p->start, storage_.begin() + part_p->start + part_p->size);
        
        int idx = 0;
        if(!disks[i].last_ok){ // 如果上次没处理完
            if(head >= part_p->start && head < part_p->start + part_p->size){ // 磁头在区间内
                idx = head - part_p->start; // 从磁头继续处理
            }
        }else{ // 上次处理完了，开始处理新区间
            disks[i].last_ok = false; // 标记本次处理未完成，且从头开始处理
        }

        // std::cerr << "idx  " << idx << std::endl;
        // std::cerr << "head  " << head << std::endl;
        // std::cerr << "part_p->start  " << part_p->start << std::endl;
        // std::cerr << "part_p->end  " << part_p->start + part_p->size << std::endl;
        // std::cerr << "real_idx  " << part_p->start + idx << std::endl;
        
        bool stop = false; // 磁头无token，stop指示
        // std::cerr << "stop " << stop << std::endl;
        for(; idx < part_p->size; ++idx){
            // std::cerr << "head " << head << std::endl;
            // std::cerr << "idx " << part_p->start+idx <<" subStorage[idx]  " << subStorage[idx] << std::endl;
            if(subStorage[idx] == 0) continue; // 该位置没有对象，继续下一个目标
            int object_id = subStorage[idx];
            
            // std::cerr << "get_request_num " << objects[object_id].get_request_num() << std::endl;
            if(objects[object_id].get_request_num() <= 0) continue; // 该对象没有请求，继续下一个目标 
            
            // std::cerr << "idx " << idx << " pass_read_rest " << disks[i].get_cur_tokens() << std::endl;
            // 找到有请求的目标，判断所需要的 行动和token
            std::pair<int, int> act_token = disks[i].get_need_token_to_head(part_p->start + idx);
            int dis = disks[i].get_distance_to_head(part_p->start + idx); // 计算距离
            
            // std::cerr << "disks " << i << std::endl;
            // std::cerr << "act_token.first " << act_token.first << std::endl;
            // std::cerr << "curr_rest_token " << disks[i].get_cur_tokens() << std::endl;
            if(act_token.first == 0){ // read
                for(int d=0; d<=dis; ++d){
                    if(disks[i].read()){ // read到指定位置然后再读取
                        printf("r"); //read成功，打印read指令
                    }
                    else {
                        printf("#\n"); //token数量不足，read失败，打印#指令，退出循环
                        stop = true;
                        break;
                    }
                }
                if(!stop){ // 成功读取该块
                    // 确定是目标的哪一块
                    int block_idx = objects[object_id].get_which_unit(i, part_p->start + idx);
                    // 记录读取进度
                    int req_id = objects[object_id].get_last_request();
                    while (req_id != 0) { // 遍历请求链表
                        // 记录进度
                        requests[req_id].set_is_done_list(block_idx);
                        if(requests[req_id].is_completed()){ // 请求完成,记录完成请求
                            if(!requests[req_id].is_up){
                                finish_requests.insert(req_id);
                                requests[req_id].is_up = true;
                                // std::cerr << "req_id " << req_id << std::endl;
                            }
                        }
                        req_id = requests[req_id].get_prev_id();
                    }
                }else{
                    break; // token数量不足，结束该硬盘操作
                }
            }
            else if(act_token.first == 1){ // pass
                for(int d=0; d<dis; ++d){
                    if(disks[i].pass()){ // pass到指定位置
                        printf("p"); //pass成功，打印pass指令
                    }
                    else {
                        printf("#\n"); //token数量不足，pass失败，打印#指令
                        stop = true;
                        break;
                    }
                    // std::cerr << "pass_rest " << disks[i].get_cur_tokens() << std::endl;
                }
                if(stop) break;
                if(disks[i].read()){ // 然后再读取
                
                    // std::cerr << "idx " << idx << " pass_read_rest " << disks[i].get_cur_tokens() << std::endl;
                    printf("r"); //read成功，打印read指令
                    // 成功读取该块，记录读取进度
                    // 确定是目标的哪一块
                    int block_idx = objects[object_id].get_which_unit(i, part_p->start + idx);
                    // 记录读取进度
                    int req_id = objects[object_id].get_last_request();
                    while (req_id != 0) { // 遍历请求链表
                        // 记录进度
                        requests[req_id].set_is_done_list(block_idx);
                        if(requests[req_id].is_completed()){ // 请求完成,记录完成请求
                            if(!requests[req_id].is_up){
                                finish_requests.insert(req_id);
                                requests[req_id].is_up = true;
                                // std::cerr << "req_id " << req_id << std::endl;
                            }
                        }
                        req_id = requests[req_id].get_prev_id();
                    }
                }
                else {
                    printf("#\n"); //token数量不足，read失败，打印#指令
                    stop = true;
                }
                if(stop) break;
            }
            else if(act_token.first == -1){ // jump
                if(disks[i].jump(part_p->start + idx)){ // jump到指定位置
                    printf("j %d\n", part_p->start + idx); //jump成功，打印pass指令
                }
                // else {
                //     // printf("#\n"); //token数量不足，jump失败，打印#指令
                //     stop = true;
                // }
                stop = true;
            }
            else if (act_token.first == -2){ // read 2jump
                // assert(false); // 不会出现
                while(disks[i].read()){ // 尽量读
                    printf("r"); //read成功，打印read指令
                }
                printf("#\n"); //token数量不足，read失败，打印#指令
                stop = true;
            }
            // std::cerr << "disk_id " << i << " ok " << disks[i].get_cur_tokens() << std::endl;
            // std::cerr << "stop " << stop << std::endl;
            // assert(false);
            if(stop) break; // token数量不足，结束该硬盘操作
        }

        if(stop){
            i++;
            continue; // token数量不足，结束该硬盘操作,继续下一个硬盘
        }

        if(idx == part_p->size){ // 完成目标块操作
            disks[i].last_ok = true;
            disks[i].set_head_free(); // 磁头空闲
            // disks[i].part_p = disks[i].part_p->next;
            // if(part_p->score > 0);
        }
    }
    // 磁头操作完毕
// 读取完成，输出读取完成的请求
    int finish_num = finish_requests.size();
    // std::cerr << "finish_num " << finish_num << std::endl;
    printf("%d\n", finish_num);
    for(const auto & req_id: finish_requests){
        // std::cerr << "req_id " << req_id << std::endl;
        printf("%d\n", req_id);
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
        delete_action(t);
        write_action(t);
        read_action(t);
    }
    // delete token_manager;

    return 0;
}