#pragma once
#include "global.h"
// #include <vector>

#define TIME_K 1.0 // 时间得分衰减系数

class Request {
private:
    int request_id;
    int object_id;
    int prev_request_id;
    bool is_done;
    bool is_read_done;
    int timestamp;
    double time_score;
    double size_score;

    // std::vector<bool> is_done_list;
    std::vector<uint64_t> is_done_bitmap; // 使用位图替代布尔数组
    int rest; // 剩余未读取的块数


public:
    bool is_up;
    Request(int req_id = 0, int obj_id = 0, int time = 0, int size = 0);
    void link_to_previous(int prev_id);
    bool is_completed() const;
    void mark_as_completed();
    int get_object_id() const;
    int get_prev_id() const;
    void set_object_id(int id);

    double compute_time_score_update(int t) const;
    double get_size_score() const;
    double get_time_score() const;
    double get_delete_prob(int t) const;
    double get_score(double t) const;
    
    void set_is_done_list(int block_idx);
    bool read_done(){
        return is_read_done;
    }
};