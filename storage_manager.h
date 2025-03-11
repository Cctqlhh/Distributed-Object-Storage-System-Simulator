#pragma once
#include <vector>
#include "token_manager.h"
#include "object.h"
#include "request.h"
#include "disk.h"

#define REP_NUM 3
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define EXTRA_TIME (105)
#define FRE_PER_SLICING 1800

// 标签统计结构
struct TagStats {
    int del_freq;   
    int write_freq; 
    int read_freq;  
    double hot_score;
    
    TagStats() : del_freq(0), write_freq(0), read_freq(0), hot_score(0) {}
    void calculate_score(int total_time) {
        hot_score = (read_freq * 3.0 + write_freq * 1.0 - del_freq * 0.5) / total_time;
    }
};

class StorageManager {
private:
    std::vector<Request>& requests;
    std::vector<Object>& objects;
    std::vector<Disk>& disks;
    std::vector<TagStats>& tag_stats;
    TokenManager* token_manager;
    int T, M, N, V, G;

    static int current_request;
    static int current_phase;

    int find_optimal_disk(int size, int tag) const;
    void process_read_request(int request_id, int object_id);
    void handle_disk_operations(int object_id);

public:
    StorageManager(std::vector<Request>& reqs, std::vector<Object>& objs, 
                  std::vector<Disk>& dsks, std::vector<TagStats>& stats,
                  int t, int m, int n, int v, int g);
    ~StorageManager();

    void init_system();
    void process_timestamp(int timestamp);
    void process_delete();
    void process_write();
    void process_read();
    void refresh_tokens();
};