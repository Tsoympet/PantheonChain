#include "metrics.h"

namespace pantheon::common {

void MetricsRegistry::Increment(const std::string& key, uint64_t amount) {
    counters_[key] += amount;
}

uint64_t MetricsRegistry::Read(const std::string& key) const {
    const auto it = counters_.find(key);
    return it == counters_.end() ? 0 : it->second;
}

}  // namespace pantheon::common
