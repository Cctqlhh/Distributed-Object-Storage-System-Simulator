#pragma once
#include "global.h"
#include "token_manager.h"

// 记录磁盘区间块的起始索引和大小
struct PartitionInfo {
    int start;  // 该区间块的起始位置（存储单元索引）
    int size;   // 该区间块的大小
    float score;  // 该区间块的优先级得分
    size_t heap_index; // 该区间块在堆中的索引
    PartitionInfo* next;  // 指向下一个区间块的指针
    // std::vector<int> need_read;  // 该区间块的每个存储单元是否需要读，需要读取的次数
    // std::vector<int> object_ids;    // 该区间块存储的所有对象id
    
    PartitionInfo() : start(0), size(0), score(0) {}  // 默认构造函数
    // PartitionInfo(int s, int sz) : start(s), size(sz), score(0), need_read(sz, 0) {}  // 带参数的构造函数
    PartitionInfo(int s, int sz) : start(s), size(sz), score(0) {}  // 带参数的构造函数
    
    // 重载 < 运算符，用于排序
    bool operator<(const PartitionInfo& other) const {
        return score < other.score;
    }
    // 重载 == 运算符，用于判断两个 PartitionInfo 对象是否相等
    bool operator==(const PartitionInfo& other) const {
        return score == other.score;
    }
};

class DynamicPartitionHeap {
    private:
        // 使用 vector 存储堆中元素（指向 PartitionInfo 对象的指针）
        std::vector<PartitionInfo*> heap;
        // 使用哈希表记录每个元素在 heap 中的索引，便于快速定位更新位置
        // std::unordered_map<PartitionInfo*, size_t> position_map;
    
        // 交换堆中两个元素的位置，并更新 position_map 映射
        void swapNodes(size_t i, size_t j) {
            std::swap(heap[i], heap[j]);
            // position_map[heap[i]] = i;
            // position_map[heap[j]] = j;
            heap[i]->heap_index = i;
            heap[j]->heap_index = j;
        }
    
        // 自下而上调整堆（上浮操作），用于元素 score 增加时
        void heapifyUp(size_t index) {
            while (index > 0) {
                size_t parent = (index - 1) / 2;
                // 这里调用 PartitionInfo 的 < 运算符，score 越高优先级越高
                if (*heap[parent] < *heap[index]) {
                    swapNodes(index, parent);
                    index = parent;
                } else {
                    break;
                }
            }
        }
    
        // 自上而下调整堆（下沉操作），用于元素 score 减小时
        void heapifyDown(size_t index) {
            size_t n = heap.size();
            while (true) {
                size_t left = 2 * index + 1;
                size_t right = 2 * index + 2;
                size_t largest = index;
                if (left < n && *heap[largest] < *heap[left]) {
                    largest = left;
                }
                if (right < n && *heap[largest] < *heap[right]) {
                    largest = right;
                }
                if (largest != index) {
                    swapNodes(index, largest);
                    index = largest;
                } else {
                    break;
                }
            }
        }
    
    public:
        DynamicPartitionHeap() {}
    
        // 插入新元素
        void push(PartitionInfo* item) {
            heap.push_back(item);
            // position_map[item] = heap.size() - 1;
            item->heap_index = heap.size() - 1;  // 设置内嵌索引
            heapifyUp(heap.size() - 1);
        }
    
        // 返回堆顶元素
        PartitionInfo* top() {
            // if (heap.empty()) return nullptr;
            // return heap[0];
            
            return heap.empty() ? nullptr : heap[0];
        }
    
        // 弹出堆顶元素
        PartitionInfo* pop() {
            if (heap.empty()) return nullptr;
            PartitionInfo* topItem = heap[0];
            PartitionInfo* last = heap.back();
            heap[0] = last;
            // position_map[last] = 0;
            last->heap_index = 0;
            heap.pop_back();
            // position_map.erase(topItem);
            if (!heap.empty()) {
                heapifyDown(0);
            }
            return topItem;
        }
    
        // 当元素内部影响排序的 score 发生变化后，调用 update 调整其在堆中的位置
        void update(PartitionInfo* item) {
            size_t index = item->heap_index;
            // auto it = position_map.find(item);
            // if (it == position_map.end()) return;  // 元素不在堆中
            // size_t index = it->second;
            // heapifyUp(index);
            // heapifyDown(index);
            if (index > 0 && *heap[(index - 1) / 2] < *heap[index]) {
                heapifyUp(index);
            } else {
                heapifyDown(index);
            }
        }
    
        bool empty() const {
            return heap.empty();
        }
    };
    

class Disk {
private:
    int id;
    int capacity;
    // 在对象写入时更新
    std::vector<int> storage;  // 存储单元，值为对象id
    int head_position;         // 磁头位置
    bool head_free;          // 磁头状态
    int max_tokens_;
    int partition_size;        // 每个分区的大小(一般)

    std::vector<int> storage_partition_map;  // 存储单元所属的分区映射（索引 1~capacity）
    
    std::vector<PartitionInfo> partitions;  // 存储每个区间块的信息（起始索引和大小）

    // 所有区间块的初始最大容量
    std::vector<int> initial_max_capacity;
    // 新增：动态更新的堆，用于管理 partitions 的指针（支持动态更新）
    DynamicPartitionHeap partition_heap;

    TokenManager token_manager;

    // 在对象写入区间块时更新
    std::vector<int> residual_capacity;     // 存储每个区间块的剩余容量，初始化为初始最大容量size

public:
    const PartitionInfo* part_p;  // 当前操作的区间块指针
    
    bool last_ok;
    Disk() : id(0), capacity(0), head_position(1), max_tokens_(0), token_manager(0) {}  // 添加默认构造函数
    Disk(int id, int capacity, int max_tokens);


    bool write(int position, int object_id) {
        storage[position] = object_id;
        return true;
    }
    void erase(int position){
        storage[position] = 0;
    }
    int get_head_position() const{
        return head_position;
    }
    bool is_free(int position) const{
        return storage[position] == 0;
    }
    int get_id() const{
        return id;
    }
    int get_capacity() const{
        return capacity;
    }
    std::vector<int> get_storage() const{
        return storage;
    }

    int get_distance_to_head(int position) const // 返回目标位置到磁头的距离（存储单元数（对象块数））== pass token消耗
    {
        assert(position > 0 && position <= capacity);
        if(position < head_position)
            return capacity - head_position + position;
        else return position - head_position;
    }
    std::pair<int, int> get_need_token_to_head(int position) const; // 返回到达目标的最优操作和token消耗。1-pass,0-read,-1-jump,-2-next-jump.

    int get_need_token_continue_read(int position) const; // 返回到达目标连续read时token消耗(包含到达后的读取)
    int get_need_token_continue_pass(int position) const; // 返回到达目标连续pass时token消耗(包含到达后的读取)
    void refresh_token_manager();

    // head操作，更新head位置并返回，同时更新token
    int jump(int position); 
    int pass();
    int read();
    
    int get_partition_id(int position) const // 计算存储单元 `position` 所属的分区
    {
        assert(position > 0 && position <= capacity);
        return storage_partition_map[position];
    }
    int get_partition_size() const // 获取磁盘的 `partition_size`
    {
        return partition_size;
    }
    // 获取某个区间块的信息,partition_id 分区编号（1~DISK_PARTITIONS）,return 该分区的 PartitionInfo 结构体
    const PartitionInfo& get_partition_info(int partition_id) const{
        assert(partition_id >= 1 && partition_id <= DISK_PARTITIONS);
        return partitions[partition_id];
    }

    int get_residual_capacity(int partition_id) const; // 获取区间块的剩余容量

    void reduce_residual_capacity(int partition_id, int size); // 减少区间块的剩余容量

    void increase_residual_capacity(int partition_id, int size); // 增加区间块的剩余容量
    void reflash_partition_score();
    // int update_partition_info(int partition_id, const Request & req);
    void update_partition_info(int partition_id, float score);
    
    bool head_is_free() const{
        return head_free;
    }
    void set_head_busy(){
        head_free = false;
    }
    void set_head_free(){
        head_free = true;
    }
    // 新增方法：初始化 partitions 和堆（例如在构造函数中调用）
    void initialize_partitions();
    // 获取堆顶（score 最高）的分区信息
    const PartitionInfo* get_top_partition();
    const PartitionInfo* get_pop_partition();
    void push_partition(PartitionInfo* partition);

    int get_cur_tokens() const{
        return token_manager.get_current_tokens();
    }

    int get_partition_start(int partition_id) const{
        return partitions[partition_id].start;
    }
    int get_partition_end(int partition_id) const{
        return partitions[partition_id].start + partitions[partition_id].size - 1;
    }
    int get_partition_size(int partition_id) const{
        return partitions[partition_id].size;
    }
};