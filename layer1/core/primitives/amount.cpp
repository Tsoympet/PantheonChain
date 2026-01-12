// ParthenonChain - Amount Implementation
// Consensus-critical: Safe arithmetic with overflow protection

#include "amount.h"
#include <sstream>
#include <iomanip>

namespace parthenon {
namespace primitives {

std::optional<Amount> Amount::Add(const Amount& other) const {
    if (WouldAddOverflow(value_, other.value_)) {
        return std::nullopt;
    }
    return Amount(value_ + other.value_);
}

std::optional<Amount> Amount::Subtract(const Amount& other) const {
    if (WouldSubtractUnderflow(value_, other.value_)) {
        return std::nullopt;
    }
    return Amount(value_ - other.value_);
}

std::optional<Amount> Amount::Multiply(uint64_t multiplier) const {
    if (WouldMultiplyOverflow(value_, multiplier)) {
        return std::nullopt;
    }
    return Amount(value_ * multiplier);
}

std::optional<Amount> Amount::Divide(uint64_t divisor) const {
    if (divisor == 0) {
        return std::nullopt;
    }
    return Amount(value_ / divisor);
}

std::string Amount::ToString() const {
    return std::to_string(value_);
}

void Amount::Serialize(uint8_t* output) const {
    // Little-endian serialization
    output[0] = static_cast<uint8_t>(value_);
    output[1] = static_cast<uint8_t>(value_ >> 8);
    output[2] = static_cast<uint8_t>(value_ >> 16);
    output[3] = static_cast<uint8_t>(value_ >> 24);
    output[4] = static_cast<uint8_t>(value_ >> 32);
    output[5] = static_cast<uint8_t>(value_ >> 40);
    output[6] = static_cast<uint8_t>(value_ >> 48);
    output[7] = static_cast<uint8_t>(value_ >> 56);
}

Amount Amount::Deserialize(const uint8_t* input) {
    // Little-endian deserialization
    uint64_t value = 0;
    value |= static_cast<uint64_t>(input[0]);
    value |= static_cast<uint64_t>(input[1]) << 8;
    value |= static_cast<uint64_t>(input[2]) << 16;
    value |= static_cast<uint64_t>(input[3]) << 24;
    value |= static_cast<uint64_t>(input[4]) << 32;
    value |= static_cast<uint64_t>(input[5]) << 40;
    value |= static_cast<uint64_t>(input[6]) << 48;
    value |= static_cast<uint64_t>(input[7]) << 56;
    return Amount(value);
}

} // namespace primitives
} // namespace parthenon
