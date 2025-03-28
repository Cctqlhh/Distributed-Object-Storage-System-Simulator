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
    , is_up(false)
    , is_choose(false) {}

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

double Request::compute_time_score_update(int t) const {
    if(is_done || is_up) return 0.0f;
    int time_ = t - timestamp;
    return (time_ <= 10) ? (1.0 - 0.005 * time_) : 
           (time_ <= 105) ? (1.05 - 0.01 * time_) : 0.0;
}

double Request::get_size_score() const{
    return size_score;
}
double Request::get_time_score() const{
    return time_score;
}
double Request::get_delete_prob(int t) const{
    return tagmanager.tag_delete_prob[objects[object_id].get_tag_id()][(t - 1) / FRE_PER_SLICING + 1];
}
double Request::get_score(int t) const{
    // double score = compute_time_score_update(t);
    // return score * size_score * (1 - get_delete_prob(t));
    // return score * size_score;

    // 1. 计算时间得分（double 精度）
    // 2. 计算对象大小得分
    double time_size_score = compute_time_score_update(t) * size_score;
    // 3. 计算不被删除的概率，添加极小值保护（避免 log(0) 或 0 乘积）
    double not_del = std::max(1e-9, 1.0 - get_delete_prob(t));

    // 设置各项权重，可根据实际调试调整（初始均设为 1.0）
    double weight_time = 1.0;
    double weight_size = 1.0;
    double weight_del  = 1.0;
    // // 使用加权乘积计算最终得分
    // double final_score = std::pow(time_score, weight_time) *
    //                      std::pow(size_score, weight_size) *
    //                      std::pow(not_del,  weight_del);
    // 如果你担心直接乘可能出现过小数值，也可以选择取对数后加权再指数化：
    double log_score = weight_time * std::log(std::max(time_score, 1e-9)) +
                       weight_size * std::log(std::max(size_score, 1e-9)) +
                       weight_del  * std::log(not_del);
    double final_score = std::exp(log_score);

    return final_score;

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