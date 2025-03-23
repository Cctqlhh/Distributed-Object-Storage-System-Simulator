#include "disk.h"


Disk::Disk(int disk_id, int disk_capacity, int max_tokens) 
    : id(disk_id)
    , capacity(disk_capacity)
    , head_position(1)
    , storage(disk_capacity + 1, 0)
    , max_tokens_(max_tokens) 
    , partition_size(std::ceil(static_cast<double>(disk_capacity) / DISK_PARTITIONS)) {
    token_manager = new TokenManager(max_tokens);

    // 初始化每个存储单元的分区信息
    storage_partition_map.resize(disk_capacity + 1);        // 存储单元编号从 1 到 disk_capacity
    partitions.resize(DISK_PARTITIONS + 1);                 // 分区编号从 1 到 20
    residual_capacity.resize(DISK_PARTITIONS + 1, 0);       // 分区编号从 1 到 20
    initial_max_capacity.resize(DISK_PARTITIONS + 1, 0);    // 分区编号从 1 到 20

    // 计算区间块的起始索引和大小
    for (int i = 1; i <= DISK_PARTITIONS; i++) {  
        int start = (i - 1) * partition_size + 1;
        int end = std::min(start + partition_size - 1, capacity); 

        partitions[i] = {start, end - start + 1}; 
        residual_capacity[i] = end - start + 1;
        initial_max_capacity[i] = end - start + 1;
    }

    // 计算存储单元所属的分区
    for (int i = 1; i <= disk_capacity; i++) {
        for (int j = 1; j <= DISK_PARTITIONS; j++) {
            if (i >= partitions[j].start && i < partitions[j].start + partitions[j].size) {
                storage_partition_map[i] = j;  // 直接匹配区间
                break;
            }
        }
        assert(storage_partition_map[i] >= 1 && storage_partition_map[i] <= DISK_PARTITIONS);  // 确保映射合法
    }
}

bool Disk::write(int position, int object_id) {
    assert(position > 0 && position <= capacity);
    storage[position] = object_id;
    return true;
}

void Disk::erase(int position) {
    assert(position > 0 && position <= capacity);
    storage[position] = 0;
}

int Disk::get_head_position() const {
    return head_position;
}

bool Disk::is_free(int position) const {
    assert(position > 0 && position <= capacity);
    return storage[position] == 0;
}

int Disk::get_id() const {
    return id;
}

int Disk::get_capacity() const {
    return capacity;
}

int Disk::get_distance_to_head(int position) const {
    assert(position > 0 && position <= capacity);
    if(position < head_position)
        return capacity - head_position + position;
    else return position - head_position;
}


std::pair<int,int> Disk::get_need_token_to_head(int position) const {
    assert(position > 0 && position <= capacity);
    // int distance = get_distance_to_head(position);
    int read_cost = get_need_token_continue_read(position);
    int pass_cost = get_need_token_continue_pass(position);
    int cur_rest_tokens = token_manager->get_current_tokens();
    int cost;
    int action;
    if(read_cost <= pass_cost){
        cost = read_cost;
        action = 0;
    }
    else{
        cost = pass_cost;
        action = 1;
    }
    // 如果是初始阶段 可选择jump
    if(cur_rest_tokens == max_tokens_){
        if(cost < max_tokens_) return {action, cost};
        return {-1, max_tokens_};
    }
    // 如果是中间阶段，不能jump
    if(cost > cur_rest_tokens + max_tokens_){
        return {-2, max_tokens_ + cur_rest_tokens};
    }
    return {action, cost};
}

int Disk::get_need_token_continue_read(int position) const{
    int distance = get_distance_to_head(position);
    // (1+(p2-p1))*readi
    int prev_read_cost_ = token_manager->get_prev_read_cost();
    int cost;
    if(!token_manager->get_last_is_read()){
        prev_read_cost_ = 64;
        cost = prev_read_cost_;
    } else {
        prev_read_cost_ = std::max(16, static_cast<int>(std::ceil(prev_read_cost_ * 0.8)));
        cost = prev_read_cost_;
    }
    for(int i = 1; i <= distance; i++){
        prev_read_cost_ = std::max(16, static_cast<int>(std::ceil(prev_read_cost_ * 0.8)));
        cost += prev_read_cost_;
    }
    return cost;
}

int Disk::get_need_token_continue_pass(int position) const{
    int distance = get_distance_to_head(position);
    int cost = distance + 64;
    return cost;
}

void Disk::refresh_token_manager(){
    token_manager->refresh();
}

int Disk::jump(int position){
    if(token_manager->consume_jump()){
        head_position = position;
        return head_position;
    } else 
        return 0;
}
int Disk::pass(){
    if(token_manager->consume_pass()){
        head_position += 1;
        if(head_position > capacity)
            head_position = 1;
        return head_position;
    }
    else return 0;
}
int Disk::read(){
    if(token_manager->consume_read()){
        head_position += 1;
        if(head_position > capacity)
            head_position = 1;
        return head_position;
    }
    else return 0;
}

int Disk::get_partition_id(int position) const {
    assert(position >= 1 && position <= capacity);
    return storage_partition_map[position];
}


int Disk::get_partition_size() const {
    return partition_size;
}

const PartitionInfo& Disk::get_partition_info(int partition_id) const {
    assert(partition_id >= 1 && partition_id <= DISK_PARTITIONS);
    return partitions[partition_id];
}

int Disk::get_residual_capacity(int partition_id) const {
    assert(partition_id >= 1 && partition_id <= DISK_PARTITIONS);
    return residual_capacity[partition_id];
}

void Disk::reduce_residual_capacity(int partition_id, int size) {
    assert(partition_id >= 1 && partition_id <= DISK_PARTITIONS);
    assert(size >= 0 && size <= residual_capacity[partition_id]);
    residual_capacity[partition_id] -= size;
    assert(residual_capacity[partition_id] <= initial_max_capacity[partition_id] && "The write operation caused the residual capacity to exceed the maximum capacity");
}

void Disk::increase_residual_capacity(int partition_id, int size) {
    assert(partition_id >= 1 && partition_id <= DISK_PARTITIONS);
    residual_capacity[partition_id] += size;
    // std::cerr<< "-------------------------: "<< (residual_capacity[partition_id] <= initial_max_capacity[partition_id]) << std::endl;
    assert(residual_capacity[partition_id] <= initial_max_capacity[partition_id] && "The deletion operation caused the remaining capacity to exceed the maximum capacity");
}
