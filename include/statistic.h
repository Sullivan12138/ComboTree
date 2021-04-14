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
    void ResetFindPos() {
        find_pos = 0;
    }
    void ResetFindGroups() {
        find_groups = 0;
    }
    void ResetCount() {
        count = 0;
    }
    size_t GetFindGroups() {
        return find_groups;
    }
    size_t GetFindPos() {
        return find_pos;
    }
    size_t GetCount() {
        return count;
    }
};

extern Stat stat;
    
} // namespace Common
