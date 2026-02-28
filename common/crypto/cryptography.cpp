#include "cryptography.h"

#include "crypto/sha256.h"

#include <iomanip>
#include <sstream>

namespace pantheon::common {

std::string PseudoSha256d(const std::string& payload) {
    const auto hash = parthenon::crypto::SHA256d::Hash256d(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const uint8_t byte : hash) {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

}  // namespace pantheon::common
