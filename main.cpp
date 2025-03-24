#include "global.h"

std::vector<Request> requests(MAX_REQUEST_NUM);
std::vector<Object> objects(MAX_OBJECT_NUM);
std::vector<Disk> disks;
int T, M, N, V, G;
// TagManager tagmanager(0, 0);
std::vector<std::vector<int>> conflict_matrix; // 冲突矩阵
std::vector<int> tag_conflict_sum; // 从 1 到 M，有 M 行
TagManager tagmanager(0, 0, 0);

int current_max_request_id = 0; // 记录目前已处理的最大请求id

void preprocess() {
    // T时间片，M标签个数，N磁盘个数，V磁盘容量，Gtoken数量
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    // 计算时间片组数
    int slicing_count = (T - 1) / FRE_PER_SLICING + 1;
    // 初始化全局 TagManager
    tagmanager = TagManager(M, N, slicing_count);
    // 数据接收
    // 存储时间片删除、写入、读取的数据
    std::vector<std::vector<int>> fre_del(M + 1, std::vector<int>(slicing_count + 1, 0));
    std::vector<std::vector<int>> fre_write(M + 1, std::vector<int>(slicing_count + 1, 0));
    std::vector<std::vector<int>> fre_read(M + 1, std::vector<int>(slicing_count + 1, 0));
    std::vector<std::vector<int>> sum(M + 1, std::vector<int>(slicing_count + 1, 0)); // 对象累积总大小
    std::vector<std::vector<int>> read_matrix(M + 1, std::vector<int>(slicing_count + 1, 0)); // 读取矩阵
    conflict_matrix.resize(M + 1, std::vector<int>(M + 1, 0)); // 冲突矩阵
    tag_conflict_sum.resize(M + 1, 0); // 标签冲突数

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

    // 计算读取矩阵（根据热度阈值）
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slicing_count; j++) {
            read_matrix[i][j] = (fre_read[i][j] >= HEAT_THRESHOLD) ? 1 : 0;
        }
    }

    // // 计算冲突矩阵1：根据读取矩阵
    // for (int a = 1; a <= M; a++) {
    //     for (int b = 1; b <= M; b++) {
    //         if (a == b) continue; // 不计算自身冲突
    //         int conflict_count = 0;
    //         for (int j = 1; j <= slicing_count; j++) {
    //             if (read_matrix[a][j] == 1 && read_matrix[b][j] == 1) {
    //                 conflict_count++;
    //             }
    //         }
    //         conflict_matrix[a][b] = conflict_count;
    //     }
    // }

    // // 计算冲突矩阵2：根据 fre_read
    // for (int a = 1; a <= M; a++) {
    //     for (int b = 1; b <= M; b++) {
    //         if (a == b) continue; // 不计算自身冲突
    //         int conflict_count = 0;
    //         for (int j = 1; j <= slicing_count; j++) {
    //             conflict_count += fre_read[a][j] + fre_read[b][j];
    //         }
    //         conflict_matrix[a][b] = conflict_count;
    //     }
    // }

    // 计算冲突矩阵3：根据 fre_read,且每 HEAT_THRESHOLD 进行量化
    for (int a = 1; a <= M; a++) {
        for (int b = 1; b <= M; b++) {
            if (a == b) continue; // 不计算自身冲突
            int conflict_count = 0;
            for (int j = 1; j <= slicing_count; j++) {
                conflict_count += fre_read[a][j] + fre_read[b][j];
            }
            conflict_matrix[a][b] = static_cast<int>(std::ceil(static_cast<double>(conflict_count) / HEAT_THRESHOLD));
        }
    }

    // 计算每个标签的冲突数
    for (int i = 1; i <= M; i++) {
        int sum = 0;
        for (int j = 1; j <= M; j++) {
            sum += conflict_matrix[i][j];
        }
        tag_conflict_sum[i] = sum;  // 从 1 到 M，有 M 行
    }


    // 初始化磁盘，把硬盘划分为二维区间块   有问题？？？
    disks.resize(N + 1);    // 磁盘数组 1-N
    for (int i = 1; i <= N; i++) {
        disks[i] = Disk(i, V, G);
    }

    // 调用全局 tagmanager 进行标签分配
    // tagmanager.calculate_tag_disk_requirement(sum, conflict_matrix, disks);
    // tagmanager.allocate_tag_disk_requirement(disks);
    tagmanager.init(sum, conflict_matrix, disks);

    // 打印---------------------------------------------------------------------
    // 遍历每个磁盘
    for (size_t disk_id = 1; disk_id < tagmanager.disk_tag_partition_num.size(); ++disk_id) {
        int total_tags = 0;
        for (const auto& tag_pair : tagmanager.disk_tag_partition_num[disk_id]) {
            total_tags += tag_pair.second;
        }
        std::cerr << "disk " << disk_id << "  total_tags: " << total_tags << std::endl;
    }

    // // 遍历每个磁盘
    // for (size_t disk_id = 1; disk_id < tagmanager.disk_tag_partition_num.size(); ++disk_id) {
    //     int total_tags = 0;
    //     for (const auto& tag_pair : tagmanager.disk_tag_partition_num[disk_id]) {
    //         std::cerr << "disk " << disk_id << " tag: "<< tag_pair.first << "  total_tags: " << tag_pair.second << std::endl;
    //     }
    // }

    tagmanager.compute_delete_prob(sum, fre_del); // 计算删除概率

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
    // 删除接收
    scanf("%d", &n_delete); // 要删除的对象个数
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]); // 要删除的对象id
    }

    // 预分配空间存储未完成的请求ID
    std::vector<int> aborted_requests;
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];    // 要删除的对象id
        if (objects[id].is_deleted_status()) continue; // 已经删除的对象不再删除
        int current_id = objects[id].get_last_request();    // 找到对象最后新增的读取请求
        while (current_id != 0) { // 遍历请求链表
            if (!requests[current_id].is_completed()) {
                abort_num++; // 统计未完成的请求个数
                aborted_requests.push_back(current_id);
            }
            current_id = requests[current_id].get_prev_id();
        }
        
        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id = objects[id].get_replica_disk_id(j);
            objects[id].delete_replica(j, disks[disk_id]); // 删除对象的每个副本
        }
        // 删除结束
        tagmanager.update_tag_info_after_delete(objects[id]); // 更新被删除对象的相关标签信息
        objects[id].mark_as_deleted();
    }

    printf("%d\n", abort_num);
    // 直接输出已收集的未完成请求ID
    for (int req_id : aborted_requests) {
        printf("%d\n", req_id);
    }
    fflush(stdout);
}

void write_action(int t)
{
    int n_write;
    scanf("%d", &n_write); // 要写的对象个数
    
    // 每个对象顺着各自写入 
    for (int i = 1; i <= n_write; i++) { // 要写的对象
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag); // 对象id,对象大小,对象标签(未保存)
        
        // 创建对象并存入对象数组
        objects[id] = Object(id, size, tag);
        
        // 计算出对象所需的位于三个不同硬盘的三个区间块
        std::vector<std::pair<int, int>> chosen_partitions; // 存储 {硬盘ID, 区间块ID}
        chosen_partitions = objects[id].select_storage_partitions(tagmanager, disks, conflict_matrix);
        
        // std::cerr<< "Object " << id << " size " << size << " tag " << tag << std::endl;
        // for (int j = 0; j < REP_NUM; j++) {
        //     std::cerr << "Object " << id << " replica " << j + 1 << " disk " << chosen_partitions[j].first << " partition " << chosen_partitions[j].second << std::endl;
        // }

        // // 写入对象
        // objects[id].write_object(disks);

        // 把对象三个副本写入到三个区间块中
        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id = chosen_partitions[j - 1].first;       // 硬盘ID
            int partition_id = chosen_partitions[j - 1].second; // 区间块ID
            const PartitionInfo& partition_info = disks[disk_id].get_partition_info(partition_id); // 区间块信息
            int start_pos = partition_info.start; // 区间块起始位置
            int end_pos = start_pos + partition_info.size - 1; // 区间块结束位置
            // 执行写入
            if (!objects[id].write_replica(j, disks[disk_id], start_pos, end_pos)) {
                assert(false && "Error: Object write failed!");
            }
        }
        // 写入结束
        tagmanager.update_tag_info_after_write(objects[id]); // 更新被写入对象的相关标签信息
        
        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", objects[id].get_replica_disk_id(j));
            assert(objects[id].get_replica_disk_id(j) >= 1 && objects[id].get_replica_disk_id(j) <= N && "Error: Invalid replica disk id!");
            for (int k = 1; k <= size; k++) {
                printf(" %d", objects[id].get_storage_position(j, k));
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

void read_update_request_info(int disk_id, int object_id, int unit_pos, std::vector<int>& finish_requests){
    if(object_id == 0) return;
    // 确定是目标的哪一块
    int block_idx = objects[object_id].get_which_unit(disk_id, unit_pos);
    auto& active_reqs = objects[object_id].get_active_requests();
    for(size_t i=0; i<active_reqs.size();){
        int req_id = active_reqs[i];
        if(requests[req_id].is_up){
            ++i;
            continue;
        }
        requests[req_id].set_is_done_list(block_idx);
        if(requests[req_id].is_completed()){
            finish_requests.push_back(req_id);  // 直接追加，无需二分查找
            requests[req_id].is_up = true;
            active_reqs[i] = active_reqs.back();
            active_reqs.pop_back();
        } else {
            ++i;
        }
    }
}

void read_action(int t)
{
// 获取读取请求
    int n_read;
    int request_id, object_id; 
    std::vector<int> finish_requests;
    scanf("%d", &n_read); // 要读的请求个数，读取几个对象
    // 添加新请求
    for (int i = 1; i <= n_read; i++) { // 可以在请求进来的时候 给硬盘分区记录 !请求变化!,新增请求和删除的请求id,然后在需要更新的时候按这个更新
        scanf("%d%d", &request_id, &object_id); // 请求id,对象id
        requests[request_id] = Request(request_id, object_id, t, objects[object_id].get_size()); // 创建request对象
        requests[request_id].link_to_previous(objects[object_id].get_last_request()); // 请求链表链接
        objects[object_id].update_last_request(request_id); // 更新对象最后新增的请求 不影响读取，只方便删除
        objects[object_id].add_active_request(request_id); // 添加到活跃请求列表
        if (request_id > current_max_request_id)
            current_max_request_id = request_id;
    }

    // 每个磁头寻找要读的对象进行读操作  
    for(int i=1; i<=N;){
        assert(i <= N && i >= 1);
        // 维护已有请求 应当磁头空闲时候进行维护
        if(disks[i].head_is_free()){ //磁头空闲，需要设置新的读取对象
            disks[i].reflash_partition_score(); // 刷新分数,以便重新写入
            bool has_request = false;
            // // 遍历硬盘所有分区对应的所有对象 请求
            // // 遍历硬盘有请求的分区
            // // 每个分区的有请求的对象
            const std::vector<int>& storage_ = disks[i].get_storage(); // 获取硬盘存储单元
            for(int partition_id = 1; partition_id <= PARTITION_ALLOCATION_THRESHOLD; ++partition_id){
                // if(disks[i].partitions[partition_id].score <= 0) continue; // 跳过分数为0的分区
                int start = disks[i].get_partition_start(partition_id);
                int size = disks[i].get_partition_size(partition_id);
                const int* subStorage = storage_.data() + start; // 获取分区的存储单元
            
                float score = 0;
                // 遍历分区的所有存储单元  // 后续替换为直接遍历分区的所有对象
                for(int idx=0; idx < size; ++idx){
                    // if(idx > 0 && subStorage[idx-1] == subStorage[idx]) continue; // 跳过连续相同的存储单元(对象)
                    int object_id = subStorage[idx];
                    if(object_id == 0) continue; // 目标不存在，则跳过
                     // 假设连续存储的,获取size之后用来跳过(后续调整为直接遍历分区的所有对象,不需要考虑size)
                    idx += objects[object_id].get_size() - 1; // 跳过连续相同的存储单元(对象)

                    // 目标存在，获取request信息 
                    auto& active_reqs = objects[object_id].get_active_requests(); // 获取对象的活跃请求列表
                    for(size_t i=0; i<active_reqs.size(); ++i){ 
                        auto & req = requests[active_reqs[i]];
                        if(req.is_up || req.is_completed()){ // 跳过已经完成的请求
                            ++i;
                            continue;
                        }
                        score += req.get_score(t);
                    }
                }
                if(!has_request && score > 0) has_request = true;
                disks[i].update_partition_info(partition_id, score); // 更新分区得分,同时更新
            }
            if(!has_request) {
                printf("#\n"); // 当前硬盘无请求，不操作
                i++;
                continue; // 下一个硬盘
            }
            // 若有请求需要处理
            disks[i].set_head_busy();
        }
        // 磁头忙（设置好了要读取的对象）进行读取操作
        int head = disks[i].get_head_position();
        // auto part_p = disks[i].get_top_partition();
        const PartitionInfo* part_p;
        if(!disks[i].last_ok) { // 上次未读取完
            // std::cerr << "last not ok" << std::endl;
            part_p = disks[i].part_p; // 继续上次区间
        }else{ // 上次读完了
            part_p = disks[i].get_pop_partition(); // 获取最高分区
            // 优化：如果最高分区分数为0，直接处理下一个磁盘
            if(part_p->score <= 0){
                disks[i].last_ok = true;
                disks[i].set_head_free();
                i++;
                continue;
            }
            if(part_p == disks[i].part_p){ // 还是原来分区
                assert(disks[i].part_p);
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
        
        // 取出存储了对象的位置vector
        const std::vector<int>& storage_ = disks[i].get_storage();
        const int* subStorage = storage_.data() + part_p->start;

        int idx = 0;
        if(!disks[i].last_ok){ // 如果上次没处理完
            if(head >= part_p->start && head < part_p->start + part_p->size){ // 磁头在区间内
                idx = head - part_p->start; // 从磁头继续处理
            }
        }else{ // 上次处理完了，开始处理新区间
            disks[i].last_ok = false; // 标记本次处理未完成，且从头开始处理
        }

        bool stop = false; // 磁头无token，stop指示
        for(; idx < part_p->size; ++idx){
            int object_id = subStorage[idx];
            if(object_id == 0) continue; // 该位置没有对象，继续下一个目标
            if(objects[object_id].get_request_num() <= 0) continue; // 该对象没有请求，继续下一个目标 
            // 找到有请求的目标，判断所需要的 行动和token
            std::pair<int, int> act_token = disks[i].get_need_token_to_head(part_p->start + idx);
            int dis = disks[i].get_distance_to_head(part_p->start + idx); // 计算距离
            
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
                    // 成功读取, 更新相关的请求信息
                    read_update_request_info(i, object_id, part_p->start + idx, finish_requests);
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
                }
                if(stop) break;
                if(disks[i].read()){ // 然后再读取
                    printf("r"); //read成功，打印read指令
                    // 成功读取该块，记录读取进度
                    read_update_request_info(i, object_id, part_p->start + idx, finish_requests);
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
                stop = true;
            }
            else if (act_token.first == -2){ // read 2jump
                while(disks[i].read()){ // 尽量读
                    printf("r"); //read成功，打印read指令
                    int temp_pos = head > 1 ? head-1 : V;
                    // int temp_obj_id = storage_[temp_pos];
                    read_update_request_info(i, storage_[temp_pos], temp_pos, finish_requests);
                }
                printf("#\n"); //token数量不足，read失败，打印#指令
                stop = true;
            }
            if(stop) break; // token数量不足，结束该硬盘操作
        }

        if(stop){
            i++;
            continue; // token数量不足，结束该硬盘操作,继续下一个硬盘
        }

        if(idx == part_p->size){ // 完成目标块操作
            disks[i].last_ok = true;
            disks[i].set_head_free(); // 磁头空闲
        }
    }
    // 磁头操作完毕
    // 读取完成，输出读取完成的请求
    printf("%d\n", finish_requests.size());
    for(const auto & req_id: finish_requests){
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