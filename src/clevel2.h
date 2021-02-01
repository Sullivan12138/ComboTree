#pragma once

#include <cstdint>
#include <libpmem.h>
#include <filesystem>
#include <atomic>
#include "kvbuffer.h"
#include "sortbuffer.h"
#include "combotree_config.h"
#include "debug.h"
#include "pmem.h"

#define READ_SIX_BYTE(addr) ((*(uint64_t*)addr) & 0x0000FFFFFFFFFFFFUL)
#define MAX_RECORD_PER_BLOCK 3
#define N_CLEVEL MAX_RECORD_PER_BLOCK
namespace combotree {

class BLevel;

// B+ tree
struct RECORD {
  std::shared_mutex lock_;
  uint64_t key;
  uint64_t value;

  DataBlock* Put(MemControl* mem, uint64_t key, uint64_t value, Node* parent);
  bool Update(MemControl* mem, uint64_t key, uint64_t value); 
  bool Get(MemControl* mem, uint64_t key, uint64_t& value) const;
  bool Delete(MemControl* mem, uint64_t key, uint64_t* value);
};

class __attribute__((packed)) CLevel {
  private:
    struct __attribute__((aligned(64))) DataBlock {
      union {
        uint16_t meta;
        struct {
          uint16_t prefix_bytes : 4;  // LSB
          uint16_t suffix_bytes : 4;
          uint16_t entries      : 4;
          uint16_t max_entries  : 4;  // MSB
        };
      };
      
      RECORD records[N_CLEVEL];
      DataBlock* next_;

      
    };  
  public:
  // allocate and persist clevel node
    class MemControl {
      private:
        std::string pmem_file_;
        void* pmem_addr_;
        size_t mapped_len_;
        uint64_t base_addr_;
        std::atomic<uintptr_t> cur_addr_;
        void* end_addr_;
        static int file_id_;

      public:
        MemControl(void* base_addr, size_t size)
          : pmem_file_(""), pmem_addr_(0), base_addr_((uint64_t)base_addr),
            cur_addr_((uintptr_t)base_addr), end_addr_((uint8_t*)base_addr+size)
        {}

        MemControl(std::string pmem_file, size_t file_size)
          : pmem_file_(pmem_file+std::to_string(file_id_++))
        {
#ifdef USE_LIBPMEM
          int is_pmem;
          std::filesystem::remove(pmem_file_);
          pmem_addr_ = pmem_map_file(pmem_file_.c_str(), file_size + 64,
                       PMEM_FILE_CREATE | PMEM_FILE_EXCL, 0666, &mapped_len_, &is_pmem);
          // assert(is_pmem == 1);
          if (pmem_addr_ == nullptr) {
            perror("CLevel::MemControl(): pmem_map_file");
            exit(1);
          }

          // aligned at 64-bytes
          base_addr_ = (uint64_t)pmem_addr_;
          if ((base_addr_ & (uintptr_t)63) != 0) {
          // not aligned
            base_addr_ = (base_addr_+64) & ~(uintptr_t)63;
          }

          cur_addr_ = base_addr_;
          end_addr_ = (uint8_t*)pmem_addr_ + mapped_len_;
#else // libvmmalloc
          pmem_file_ = "";
          pmem_addr_ = nullptr;
          base_addr_ = (uint64_t)new (std::align_val_t{64}) uint8_t[file_size];
          assert((base_addr_ & 63) == 0);
          cur_addr_  = base_addr_;
          end_addr_  = (uint8_t*)base_addr_ + file_size;
#endif
        }
        
        DataBlock* newDataBlock(int suffix_len) {
          assert(suffix_len > 0 && suffix_len <= 8);
          assert((uint8_t*)cur_addr_.load() + sizeof(DataBlock) < end_addr_);
          DataBlock* ret = (DataBlock*)cur_addr_.fetch_add(sizeof(DataBlock));
          
          ret->leaf_buf.suffix_bytes = suffix_len;
          ret->leaf_buf.prefix_bytes = 8 - suffix_len;
          ret->leaf_buf.entries = 0;
          ret->leaf_buf.max_entries = ret->leaf_buf.MaxEntries();
          return ret;
        }

        uint64_t BaseAddr() const {
          return base_addr_;
        }

        uint64_t Usage() const {
          return (uint64_t)cur_addr_.load() - base_addr_;
        }

    };



  ALWAYS_INLINE uint64_t key() const {
    return cur_->leaf_buf.sort_key(idx_, prefix_key);
  }

  ALWAYS_INLINE uint64_t value() const {
    return cur_->leaf_buf.sort_value(idx_);
  }

  // return false if reachs end
  ALWAYS_INLINE bool next() {
    idx_++;
    if (idx_ >= cur_->leaf_buf.entries) {
      do {
        cur_ = (const Node*)cur_->GetNext(mem_->BaseAddr());
      } while (cur_ != nullptr && cur_->leaf_buf.Empty());
      idx_ = 0;
      return cur_ == nullptr ? false : true;
    } else {
      return true;
    }
  }

  ALWAYS_INLINE bool end() const {
    return cur_ == nullptr ? true : false;
  }

  private:
    const MemControl* mem_;
    uint64_t prefix_key;
    const Node* cur_;
    int idx_;   // current index in node
};

  CLevel();
  ALWAYS_INLINE bool HasSetup() const { return !(root_[0] & 1); };
  void Setup(MemControl* mem, int suffix_len);
  void Setup(MemControl* mem, KVBuffer<48+64,8>& buf);
  bool Put(MemControl* mem, uint64_t key, uint64_t value);

  ALWAYS_INLINE bool Update(MemControl* mem, uint64_t key, uint64_t value) {
    return root(mem->BaseAddr())->Update(mem, key, value);
  }

  ALWAYS_INLINE bool Get(MemControl* mem, uint64_t key, uint64_t& value) const {
    return root(mem->BaseAddr())->Get(mem, key, value);
  }

  ALWAYS_INLINE bool Delete(MemControl* mem, uint64_t key, uint64_t* value) {
    return root(mem->BaseAddr())->Delete(mem, key, value);
  }

 private:
  struct Node;

  uint8_t root_[6]; // uint48_t, LSB == 1 means NULL

  ALWAYS_INLINE Node* root(uint64_t base_addr) const {
    return (Node*)(READ_SIX_BYTE(root_) + base_addr);
  }
};

static_assert(sizeof(CLevel) == 6, "sizeof(CLevel) != 6");

} // namespace combotree

#undef READ_SIX_BYTE