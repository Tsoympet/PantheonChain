#include "storage.h"

namespace pantheon::common {

void KeyValueStorage::Put(const std::string& key, const std::string& value) {
    values_[key] = value;
}

std::optional<std::string> KeyValueStorage::Get(const std::string& key) const {
    const auto it = values_.find(key);
    if (it == values_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace pantheon::common
