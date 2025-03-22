#pragma once

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <cmath> // 用于 ceil 计算
#include <algorithm>  // 用于排序
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <limits>

#include <thread>
#include <mutex>

// 前向声明
class Disk;
class Request;
class Object;
class TagManager;

#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define HEAT_THRESHOLD 25000  // 设定热度阈值
#define PARTITION_ALLOCATION_THRESHOLD 20  // 选取第20个时间片组进行分配
#define REP_NUM 3

extern std::vector<Request> requests;
extern std::vector<Object> objects;
extern std::vector<Disk> disks;
extern int T, M, N, V, G;
extern TagManager tagmanager;

#include "object.h"
#include "request.h"
#include "disk.h"
#include "tag_manager.h"
// #include "token_manager.h"