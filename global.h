#pragma once

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <cmath> // 用于 ceil 计算
#include <algorithm> // 用于排序
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <set>
#include <map>
#include <limits>

// #include <thread>
// #include <mutex>

// 前向声明
class Disk;
class Request;
class Object;
class TagManager;
// class TokenManager;

#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define HEAT_THRESHOLD 25000 // 设定热度阈值
#define PARTITION_ALLOCATION_THRESHOLD 20 // 选取第20个时间片组进行分配
#define REP_NUM 3
#define DISK_PARTITIONS 20  // 定义硬盘分区数
#define SCALE 0.3                           // 选择初始存储需求的比例因子
#define MAX_PARTITIONS_PER_TAG 10           // 同一个标签在一个磁盘上的最多区间块数


extern std::vector<Request> requests;
extern std::vector<Object> objects;
extern std::vector<Disk> disks;
extern int T, M, N, V, G;
extern TagManager tagmanager;
extern std::vector<std::vector<int>> conflict_matrix; // 冲突矩阵
extern std::vector<int> tag_conflict_sum; // 从 1 到 M，有 M 行

#include "object.h"
#include "request.h"
#include "disk.h"
#include "tag_manager.h"
// #include "token_manager.h"
