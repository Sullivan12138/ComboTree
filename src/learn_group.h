#pragma once

#include <atomic>
#include <cstdint>
#include <iostream>
#include <cstddef>
#include <vector>
#include <shared_mutex>
#include "combotree_config.h"
#include "kvbuffer.h"
#include "clevel.h"
#include "pmem.h"

#include "bentry.h"
#include "common_time.h"
#include "pointer_bentry.h"
#include "learnindex/learn_index.h"

namespace combotree {

static inline int CommonPrefixBytes(uint64_t a, uint64_t b) {
  // the result of clz is undefined if arg is 0
  int leading_zero_cnt = (a ^ b) == 0 ? 64 : __builtin_clzll(a ^ b);
  return leading_zero_cnt / 8;
}

struct __attribute__((aligned(64))) LearnGroup {

    static const size_t epsilon = 4;
    static const size_t max_entry_counts = 256;
    typedef PGM_NVM::PGMIndex<uint64_t, epsilon>::Segment segment_t;
    typedef combotree::PointerBEntry bentry_t;
    class EntryIter;

    int entry_count;
    uint64_t entries_offset_;                     // pmem file offset
    segment_t segment_;
    bentry_t* __attribute__((aligned(64))) entries_; // current mmaped address
    size_t nr_entries_;  
    size_t max_nr_entries_;                        // logical entries count
    std::atomic<size_t> size_;
    CLevel::MemControl *clevel_mem_;

LearnGroup(size_t max_size, CLevel::MemControl *clevel_mem) 
    : nr_entries_(0), max_nr_entries_(max_size), clevel_mem_(clevel_mem) {
    entries_ = (bentry_t *)NVM::data_alloc->alloc(max_size * sizeof(bentry_t));
}

~LearnGroup()
{
    NVM::data_alloc->Free(entries_);
}

uint64_t start_key()
{
    return entries_[0].entry_key;
}


int find_near_pos(uint64_t key) const
{
    return segment_(key);
}

uint64_t Find_(uint64_t key) const {
    int pos = find_near_pos(key);
    pos = std::min(pos, (int)(nr_entries_ - 1));
    int left = 0;
    int right = nr_entries_ - 1;
    if(entries_[pos].entry_key == key) {
        return pos;
    } else if (entries_[pos].entry_key < key) {
      pos ++;
      for(; pos <= right && entries_[pos].entry_key <= key; pos ++);
      return pos - 1;
    } else {
      pos --;
      for(; pos > 0 && entries_[pos].entry_key > key; pos --);
      return pos >= 0 ? pos : 0;
    }
}

status Put(uint64_t key, uint64_t value)
{
    uint64_t pos = Find_(key);
    return entries_[pos].Put(clevel_mem_, key, value);
}

bool Update(uint64_t key, uint64_t value, uint64_t begin, uint64_t end);

bool Get(uint64_t key, uint64_t& value) const {
    uint64_t pos = Find_(key);
    return entries_[pos].Get(clevel_mem_, key, value);
}
bool Delete(uint64_t key, uint64_t* value, uint64_t begin, uint64_t end);

void ExpandPut_(const PointerBEntry::entry *entry) {
    new (&entries_[nr_entries_ ++]) bentry_t(entry);
}

void Persist()
{
    NVM::Mem_persist(entries_, sizeof(bentry_t) * nr_entries_);
    NVM::Mem_persist(this, sizeof(LearnGroup));
}

void Expansion(std::vector<std::pair<uint64_t,uint64_t>>& data, size_t start_pos, int &expand_keys)
{
    pgm::internal::OptimalPiecewiseLinearModel<uint64_t, uint64_t> opt(epsilon);
    size_t c = 0;
    size_t start = 0;
    expand_keys = 0;
    for (size_t i = start_pos; i < data.size(); ++i) {
        int prefix_len = CommonPrefixBytes(data[i].first, (i == data.size() - 1) ? UINT64_MAX : data[i+1].first);
        if ((!opt.add_point(data[i].first, i - start_pos)) || expand_keys >= max_nr_entries_) {
            segment_ = segment_t(opt.get_segment());
            break;
        }
        new (&entries_[i - start_pos]) bentry_t(data[i].first, data[i].second, prefix_len, clevel_mem_);
        expand_keys ++;
    }
    nr_entries_ = expand_keys;
}

bool IsKeyExpanded(uint64_t key, int& range, uint64_t& end) const;
void PrepareExpansion(LearnGroup* old_group);
void Expansion(LearnGroup* old_group);

};

class LearnGroup::EntryIter {
public:
    EntryIter(LearnGroup* group) : group_(group), cur_idx(0) {
        new (&biter_) bentry_t::EntryIter(&group_->entries_[cur_idx]);
    }
    const PointerBEntry::entry& operator*() { return *biter_; }


    ALWAYS_INLINE bool next() {
        if(cur_idx < group_->nr_entries_ && biter_.next())
        {
            return true;
        }
        cur_idx ++;
        if(cur_idx >= group_->nr_entries_) {
            return false;
        }
        new (&biter_) bentry_t::EntryIter(&group_->entries_[cur_idx]);
        return true;
    }

      ALWAYS_INLINE bool end() const {
        return cur_idx >= group_->nr_entries_;
      }
private:
    LearnGroup* group_;
    bentry_t::EntryIter biter_;
    uint64_t cur_idx;
};

static inline void ExpandGroup(LearnGroup* old_group, std::vector<LearnGroup*> &new_groups, 
        CLevel::MemControl *clevel_mem)
{
    LearnGroup::EntryIter it(old_group);
    // for(size_t i = 0; i < old_group->nr_entries_; i ++) {
    //     std::cout << "Entry[" << i << "] pointesr :" << old_group->entries_[i].buf.entries << std::endl;
    // }
    while(!it.end()) {
        pgm::internal::OptimalPiecewiseLinearModel<uint64_t, uint64_t> opt(LearnGroup::epsilon);
        size_t entry_pos = 0;
        LearnGroup *new_group = new (NVM::data_alloc->alloc(sizeof(LearnGroup))) LearnGroup(LearnGroup::max_entry_counts, clevel_mem);
        do {
            const PointerBEntry::entry *entry = &(*it);
            if ((!opt.add_point(entry->entry_key, entry_pos)) || entry_pos >= LearnGroup::max_entry_counts) {
                new_group->segment_ = LearnGroup::segment_t(opt.get_segment());
                break;
            }
            new_group->ExpandPut_(entry);
            it.next(); 
            entry_pos ++;
        } while(!it.end());
        new_groups.push_back(new_group);
    }
    // std::cout << "Expand segmets to: " << new_groups.size() << std::endl;
}

class RootModel {
public:
    class IndexIter;
    RootModel(size_t max_groups, CLevel::MemControl *clevel_mem) 
        : nr_groups_(0), max_groups_(max_groups), clevel_mem_(clevel_mem)
    {
        groups_ = (LearnGroup **)NVM::data_alloc->alloc(max_groups * sizeof(LearnGroup *));

    }
    ~RootModel() {
       NVM::data_alloc->Free(groups_, max_groups_ * sizeof(LearnGroup *));
    }

    void Load(std::vector<std::pair<uint64_t,uint64_t>>& data) {
        size_t size = data.size();
        size_t start_pos = 0;
        int expand_keys;
        std::vector<uint64_t> train_keys;
        while(start_pos < size ) {
            LearnGroup *new_group = new (NVM::data_alloc->alloc(sizeof(LearnGroup))) LearnGroup(LearnGroup::max_entry_counts, clevel_mem_);
            new_group->Expansion(data, start_pos, expand_keys);
            start_pos += expand_keys;
            groups_[nr_groups_ ++] = new_group;
            train_keys.push_back(new_group->start_key());
        }
        LOG(Debug::INFO, "Group count: %d...", nr_groups_);
        assert(nr_groups_ <=  max_groups_);
        model.prepare_model(train_keys, 0, nr_groups_);
    }

    void ExpandEntrys(std::vector<LearnGroup *> &expand_groups, size_t group_id)
    {
        size_t start_pos = 0;
        int expand_keys;
        if(nr_groups_ + expand_groups.size() < max_groups_) {
            if(group_id + 1 < nr_groups_) {
            memmove(&groups_[group_id + expand_groups.size()], &groups_[group_id + 1], 
                sizeof(LearnGroup *) * (nr_groups_ - group_id - 1 ));
            }
            nr_groups_ += expand_groups.size() - 1;
            std::copy(expand_groups.begin(), expand_groups.end(), &groups_[group_id]);
        } else {
            std::cout << "New entrys" << std::endl;
            LearnGroup **old_groups = groups_;
            size_t old_cap = max_groups_;
            size_t new_cap = (nr_groups_ + expand_groups.size()) * 2;
            LearnGroup **new_groups_ = (LearnGroup **)NVM::data_alloc->alloc(new_cap * sizeof(LearnGroup *));
            std::copy(&groups_[0], &groups_[group_id], &new_groups_[0]);
            std::copy(expand_groups.begin(), expand_groups.end(), &new_groups_[group_id]);
            std::copy(&groups_[group_id + 1], &groups_[nr_groups_], &new_groups_[group_id + expand_groups.size()]);

            max_groups_ = new_cap;
            nr_groups_ += expand_groups.size() - 1;
            groups_ = new_groups_;
            NVM::data_alloc->Free(old_groups, old_cap * sizeof(LearnGroup *));
        }
        std::vector<uint64_t> train_keys;
        for(int i = 0; i < nr_groups_; i ++) {
            train_keys.push_back(groups_[i]->start_key());
        }
        model.prepare_model(train_keys, 0, nr_groups_);
    }

    int FindGroup(uint64_t key) const {
        int pos = model.predict(key);
        pos = std::min(pos, (int)nr_groups_ - 1);
        int left = 0;
        int right = nr_groups_ - 1;
        if(groups_[pos]->start_key() == key) {
            return pos;
        } else if (groups_[pos]->start_key() < key) {
            pos ++;
            for(; pos <= right && groups_[pos]->start_key() <= key; pos ++);
            pos --;
        } else {
            pos --;
            for(; pos > 0 && groups_[pos]->start_key() > key; pos --);
        }
        return std::max(pos, 0);
    }

    status Put(uint64_t key, uint64_t value) {
    retry0:
        int group_id = FindGroup(key);
    retry1:
        auto ret = groups_[group_id]->Put(key, value);
        if(ret == status::Full ) {
            // std::cout << "Full group. need expand." << std::endl;
            LearnGroup *old_group = groups_[group_id];
            std::vector<LearnGroup *> expand_groups;
            ExpandGroup(old_group, expand_groups, clevel_mem_);
            if(expand_groups.size() == 1) {
                groups_[group_id] = expand_groups[0];
                clflush(&groups_[group_id]);
                goto retry1;
            } else {
                std::cout << "Need expand group entrys." << std::endl;
                ExpandEntrys(expand_groups, group_id);
                goto retry0;
            }
        }
        return ret;
    }
    bool Update(uint64_t key, uint64_t value);

    bool Get(uint64_t key, uint64_t& value) const {
        int group_id = FindGroup(key);
        return groups_[group_id]->Get(key, value);
    }
    bool Delete(uint64_t key);


private:
    uint64_t nr_groups_;
    uint64_t max_groups_;
    RMI::LinearModel<RMI::Key_64> model;
    LearnGroup **groups_;
    CLevel::MemControl *clevel_mem_;
};

class RootModel::IndexIter
{
public:
    using difference_type = ssize_t;
    using value_type = const uint64_t;
    using pointer = const uint64_t *;
    using reference = const uint64_t &;
    using iterator_category = std::random_access_iterator_tag;

    IndexIter(RootModel *root) : root_(root) { }
    IndexIter(RootModel *root, uint64_t idx) : root_(root), idx_(idx) { }
    ~IndexIter() {

    }
    uint64_t operator*()
    { return root_->groups_[idx_]->start_key(); }

    IndexIter& operator++()
    { idx_ ++; return *this; }

    IndexIter operator++(int)         
    { return IndexIter(root_, idx_ ++); }

    IndexIter& operator-- ()  
    { idx_ --; return *this; }

    IndexIter operator--(int)         
    { return IndexIter(root_, idx_ --); }

    uint64_t operator[](size_t i) const
    {
        if((i + idx_) > root_->nr_groups_)
        {
        std::cout << "索引超过最大值" << std::endl; 
        // 返回第一个元素
        return root_->groups_[0]->start_key();
        }
        return root_->groups_[i + idx_]->start_key();
    }
    
    bool operator<(const IndexIter& iter) const { return  idx_ < iter.idx_; }
    bool operator==(const IndexIter& iter) const { return idx_ == iter.idx_ && root_ == iter.root_; }
    bool operator!=(const IndexIter& iter) const { return idx_ != iter.idx_ || root_ != iter.root_; }
    bool operator>(const IndexIter& iter) const { return  idx_ < iter.idx_; }
    bool operator<=(const IndexIter& iter) const { return *this < iter || *this == iter; }
    bool operator>=(const IndexIter& iter) const { return *this > iter || *this == iter; }
    size_t operator-(const IndexIter& iter) const { return idx_ - iter.idx_; }

    const IndexIter& base() { return *this; }

private:
    RootModel *root_;
    uint64_t idx_;
};

} // namespace combotree