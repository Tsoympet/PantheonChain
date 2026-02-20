#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace pantheon::common {

class MetricsRegistry {
  public:
    void Increment(const std::string& key, uint64_t amount = 1);
    uint64_t Read(const std::string& key) const;

  private:
    std::unordered_map<std::string, uint64_t> counters_;
};

}  // namespace pantheon::common
