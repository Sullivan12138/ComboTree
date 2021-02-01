#include <iostream>
#include "combotree_config.h"
#include "clevel.h"

namespace combotree {

typedef std::shared_lock<std::shared_mutex> ReadLock;
typedef std::lock_guard<std::shared_mutex> WriteLock;


bool CLevel::Record::Read(uint8_t& value) const {
    ReadLock lock(this->lock_);
    *value = this->value;
}

bool CLevel::Record::Write(uint8_t value) const {
    WriteLock lock(this->lock_);
    this->value = value;
}
} // namespace combotree