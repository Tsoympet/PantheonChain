#pragma once

#include <deque>
#include <string>

namespace pantheon::common {

class Mempool {
  public:
    void Add(const std::string& tx_id);
    bool Contains(const std::string& tx_id) const;
    std::string PopFront();
    size_t Size() const;

  private:
    std::deque<std::string> tx_ids_;
};

}  // namespace pantheon::common
