#include "cryptography.h"

#include <functional>
#include <iomanip>
#include <sstream>

namespace pantheon::common {

std::string PseudoSha256d(const std::string& payload) {
    const std::hash<std::string> hasher;
    const auto first = hasher(payload);
    const auto second = hasher(std::to_string(first));

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << first << std::setw(16) << second;
    const auto digest = stream.str();
    return digest + digest;
}

}  // namespace pantheon::common
