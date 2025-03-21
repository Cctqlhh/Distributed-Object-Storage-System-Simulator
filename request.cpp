#include "request.h"

Request::Request(int req_id, int obj_id, int time,int size)
    : request_id(req_id)
    , object_id(obj_id)
    , prev_request_id(0)
    , is_done(false)
    , timestamp(time)
    , time_score(TIME_K * time)
    , size_score(0.5 * (size + 1))
    // , is_done_list(size + 1, false)
    , is_done_bitmap((size + 63) / 64, 0) // 每64位一个uint64_t
    , rest(size)
    , is_up(false) {}

void Request::link_to_previous(int prev_id) {
    prev_request_id = prev_id;
}

bool Request::is_completed() const {
    return is_done;
}

void Request::mark_as_completed() {
    if(objects[object_id].is_deleted_status()) return;
    objects[object_id].reduce_request();
    is_done = true;
}

int Request::get_object_id() const {
    return object_id;
}

int Request::get_prev_id() const {
    return prev_request_id;
}

void Request::set_object_id(int id) {
    object_id = id;
}

float Request::compute_time_score_update(int t) const {
    if(is_done) return 0;
    int time_ = t - timestamp;
    // if(time_ <= 10){
    //     return 1.0 - 0.005 * time_;
    // }
    // else if(time_ <= 105){
    //     return 1.05 - 0.01 * time_;
    // }
    // else return 0;
    // 使用条件运算符简化逻辑
    return (time_ <= 10) ? (1.0f - 0.005f * time_) : 
           (time_ <= 105) ? (1.05f - 0.01f * time_) : 0.0f;
}

float Request::get_size_score() const{
    if(is_done) return 0;
    return size_score;
}
float Request::get_time_score() const{
    return time_score;
}

void Request::set_is_done_list(int block_idx){
    // if(!is_done_list[block_idx]){ 
    //     is_done_list[block_idx] = true;
    //     rest--;
    //     if(rest == 0){
    //         mark_as_completed();
    //     }
    // }
    int bitmap_idx = block_idx / 64;
    int bit_pos = block_idx % 64;
    uint64_t mask = 1ULL << bit_pos;
    
    if(!(is_done_bitmap[bitmap_idx] & mask)){ 
        is_done_bitmap[bitmap_idx] |= mask;
        rest--;
        if(rest == 0){
            mark_as_completed();
        }
    }
}