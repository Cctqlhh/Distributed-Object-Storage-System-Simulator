#pragma once
#include "global.h"
// #include <cmath>
// #include <vector>
// #include <iostream>

class TokenManager {
private:
    int max_tokens_;        // 每个磁头最大令牌数(G)
    int current_tokens_;    // 每个磁头当前剩余令牌数
    int prev_read_cost_;    // 每个磁头上次read操作的令牌消耗
    bool last_is_read_;     // 每个磁头上次操作是否为read


public:
    TokenManager(int max_tokens) : 
        max_tokens_(max_tokens),
        current_tokens_(max_tokens),
        prev_read_cost_(0),
        last_is_read_(false) {}
    
    ~TokenManager() = default;
    // 刷新令牌（每个时间片调用）
    void refresh(){
        current_tokens_ = max_tokens_;
    }
    // 尝试消耗指定磁头的令牌
    bool try_consume(int tokens) {
        if (current_tokens_ >= tokens) {
            // std::cerr << "current_tokens_" << current_tokens_ << " ok" << std::endl;
            current_tokens_ -= tokens;
            return true;
        }
        // std::cerr << "current_tokens_" << current_tokens_ << " no" << std::endl;
        return false;
    }

    // jump操作消耗
    bool consume_jump() {
        if (try_consume(max_tokens_)) {
            last_is_read_ = false;
            return true;
        }
        return false;
    }

    // pass操作消耗
    bool consume_pass() {
        if (try_consume(1)) {
            last_is_read_ = false;
            return true;
        }
        return false;
    }

    // read操作消耗
    bool consume_read() {
        int new_cost;  // 临时存储计算出的新消耗值
        if (!last_is_read_) {
            new_cost = 64;
        } else {
            new_cost = std::max(16, static_cast<int>(std::ceil(prev_read_cost_ * 0.8)));
        }

        if (try_consume(new_cost)) {
            prev_read_cost_ = new_cost;
            last_is_read_ = true;
            return true;
        }
        return false;
    }

    // 获取当前令牌数（用于调试）
    int get_current_tokens() const {
        return current_tokens_;
    }

    // 获取上次read操作消耗（用于调试）
    int get_prev_read_cost() const {
        return prev_read_cost_;
    }

    bool get_last_is_read() const {
        return last_is_read_;
    }
};