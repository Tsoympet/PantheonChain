#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace pantheon::common {

class KeyValueStorage {
  public:
    void Put(const std::string& key, const std::string& value);
    std::optional<std::string> Get(const std::string& key) const;

  private:
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace pantheon::common
