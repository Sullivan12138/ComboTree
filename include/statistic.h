#pragma once
#include <cstdint>

namespace Common
{
// 这里以后可能要加锁
struct Stat {
    size_t find_groups;
    size_t find_pos;
    size_t count;

    void AddCount() {
        count ++;
    }
    void AddFindGroup() {
        find_groups ++;
    }

    void AddFindPos() {
        find_pos ++;
    }
};

extern Stat stat;
    
} // namespace Common
