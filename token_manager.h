#pragma once
#include "global.h"

class TokenManager {
private:
    int max_tokens_;        // 每个磁头最大令牌数(G)
    std::vector<int> ex_tokens_; 
    // int current_tokens_;    // 每个磁头当前剩余令牌数
    // int current_tokens_;    // 每个磁头当前剩余令牌数
    std::vector<int> current_tokens_; // 每个磁头的令牌数
    int prev_read_cost_;    // 每个磁头上次read操作的令牌消耗
    bool last_is_read_;     // 每个磁头上次操作是否为read


public:
    TokenManager(int max_tokens, std::vector<int> ex_tokens) : 
        max_tokens_(max_tokens),
        ex_tokens_(ex_tokens),
        current_tokens_(2, max_tokens + ex_tokens[1]),
        prev_read_cost_(0),
        last_is_read_(false) {}
    
    ~TokenManager() = default;
    // 刷新令牌（每个时间片调用）
    void refresh(int t){
        // current_tokens_ = max_tokens_ + ex_tokens_[(t - 1) / FRE_PER_SLICING + 1];
        current_tokens_ = std::vector<int>(2, max_tokens_ + ex_tokens_[ceil(t/1800.0)]);
    }
    // 尝试消耗指定磁头的令牌
    bool try_consume(int tokens, int j) {
        if (current_tokens_[j] >= tokens) {
            // std::cerr << "current_tokens_" << current_tokens_ << " ok" << std::endl;
            current_tokens_[j] -= tokens;
            return true;
        }
        // std::cerr << "current_tokens_" << current_tokens_ << " no" << std::endl;
        return false;
    }

    // jump操作消耗
    bool consume_jump(int t, int j) {
        
        // int tt = (t - 1) / FRE_PER_SLICING + 1;
        if (try_consume(max_tokens_ + ex_tokens_[ceil(t/1800.0)], j)) {
            last_is_read_ = false;
            return true;
        }
        return false;
    }

    // pass操作消耗
    bool consume_pass(int j) {
        if (try_consume(1, j)) {
            last_is_read_ = false;
            return true;
        }
        return false;
    }

    // read操作消耗
    bool consume_read(int j) {
        int new_cost;  // 临时存储计算出的新消耗值
        if (!last_is_read_) {
            new_cost = 64;
        } else {
            new_cost = std::max(16, static_cast<int>(std::ceil(prev_read_cost_ * 0.8)));
        }

        if (try_consume(new_cost, j)) {
            prev_read_cost_ = new_cost;
            last_is_read_ = true;
            return true;
        }
        return false;
    }

    // 获取当前令牌数（用于调试）
    int get_current_tokens(int j) const {
        return current_tokens_[j];
    }

    // 获取上次read操作消耗（用于调试）
    int get_prev_read_cost() const {
        return prev_read_cost_;
    }

    bool get_last_is_read() const {
        return last_is_read_;
    }
};