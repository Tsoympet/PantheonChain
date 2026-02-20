#include "mempool.h"

#include <algorithm>

namespace pantheon::common {

void Mempool::Add(const std::string& tx_id) {
    if (!Contains(tx_id)) {
        tx_ids_.push_back(tx_id);
    }
}

bool Mempool::Contains(const std::string& tx_id) const {
    return std::find(tx_ids_.begin(), tx_ids_.end(), tx_id) != tx_ids_.end();
}

std::string Mempool::PopFront() {
    if (tx_ids_.empty()) {
        return "";
    }
    const std::string front = tx_ids_.front();
    tx_ids_.pop_front();
    return front;
}

size_t Mempool::Size() const {
    return tx_ids_.size();
}

}  // namespace pantheon::common
