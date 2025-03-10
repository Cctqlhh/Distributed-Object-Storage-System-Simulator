#pragma once
#include <cmath>
#include <vector>

class TokenManager {
private:
    int max_tokens_;        // 每个磁头最大令牌数(G)
    std::vector<int> current_tokens_;    // 每个磁头当前剩余令牌数
    std::vector<int> prev_read_cost_;    // 每个磁头上次read操作的令牌消耗
    std::vector<bool> last_is_read_;     // 每个磁头上次操作是否为read
    int disk_num_;         // 硬盘数量

    bool is_valid_disk_id(int disk_id) const {
        return disk_id > 0 && disk_id <= disk_num_;
    }

public:
    TokenManager(int max_tokens, int disk_num) : 
        max_tokens_(max_tokens),
        disk_num_(disk_num),
        current_tokens_(disk_num + 1, max_tokens),
        prev_read_cost_(disk_num + 1, 0),
        last_is_read_(disk_num + 1, false) {}
    
    // 刷新令牌（每个时间片调用）
    void refresh() {
        for (int i = 1; i <= disk_num_; i++) {
            current_tokens_[i] = max_tokens_;
        }
    }

    ~TokenManager() = default;

    // 尝试消耗指定磁头的令牌
    bool try_consume(int disk_id, int tokens) {
        if (!is_valid_disk_id(disk_id)) {
            return false;
        }
        if (current_tokens_[disk_id] >= tokens) {
            current_tokens_[disk_id] -= tokens;
            return true;
        }
        return false;
    }

    // jump操作消耗
    bool consume_jump(int disk_id) {
        if (!is_valid_disk_id(disk_id)) {
            return false;
        }
        if (try_consume(disk_id, max_tokens_)) {
            last_is_read_[disk_id] = false;
            return true;
        }
        return false;
    }

    // pass操作消耗
    bool consume_pass(int disk_id) {
        if (!is_valid_disk_id(disk_id)) {
            return false;
        }
        if (try_consume(disk_id, 1)) {
            last_is_read_[disk_id] = false;
            return true;
        }
        return false;
    }

    // read操作消耗
    bool consume_read(int disk_id) {
        if (!is_valid_disk_id(disk_id)) {
            return false;
        }
        int new_cost;  // 临时存储计算出的新消耗值
        if (!last_is_read_[disk_id]) {
            new_cost = 64;
        } else {
            new_cost = std::max(16, static_cast<int>(std::ceil(prev_read_cost_[disk_id] * 0.8)));
        }

        if (try_consume(disk_id, new_cost)) {
            prev_read_cost_[disk_id] = new_cost;
            last_is_read_[disk_id] = true;
            return true;
        }
        return false;
    }

    // 获取当前令牌数（用于调试）
    int get_current_tokens(int disk_id) const {
        if (!is_valid_disk_id(disk_id)) {
            return -1;
        }
        return current_tokens_[disk_id];
    }

    // 获取上次read操作消耗（用于调试）
    int get_prev_read_cost(int disk_id) const {
        if (!is_valid_disk_id(disk_id)) {
            return -1;
        }
        return prev_read_cost_[disk_id];
    }
};