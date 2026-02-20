// ParthenonChain - Amount Handling
// Consensus-critical: Safe arithmetic with overflow protection
// All amounts are 64-bit unsigned integers (satoshi-style)

#ifndef PARTHENON_PRIMITIVES_AMOUNT_H
#define PARTHENON_PRIMITIVES_AMOUNT_H

#include <cstddef>  // Workaround for macOS Xcode 16.4 ptrdiff_t issue
#include <cstdint>
#include <optional>
#include <string>

namespace parthenon {
namespace primitives {

/**
 * Amount represents a quantity of any asset in the system
 * Uses 64-bit unsigned integers to prevent negative amounts
 * All operations check for overflow and return std::nullopt on error
 *
 * Consensus-critical: Any change to arithmetic must maintain compatibility
 */
class Amount {
  public:
    // Maximum representable amount (2^64 - 1)
    static constexpr uint64_t MAX_AMOUNT = UINT64_MAX;

    // Default constructor (zero amount)
    Amount() : value_(0) {}

    // Construct from uint64_t
    explicit Amount(uint64_t value) : value_(value) {}

    // Get raw value
    uint64_t GetValue() const { return value_; }

    // Comparison operators
    bool operator==(const Amount& other) const { return value_ == other.value_; }
    bool operator!=(const Amount& other) const { return value_ != other.value_; }
    bool operator<(const Amount& other) const { return value_ < other.value_; }
    bool operator<=(const Amount& other) const { return value_ <= other.value_; }
    bool operator>(const Amount& other) const { return value_ > other.value_; }
    bool operator>=(const Amount& other) const { return value_ >= other.value_; }

    // Safe addition with overflow check
    std::optional<Amount> Add(const Amount& other) const;

    // Safe subtraction with underflow check
    std::optional<Amount> Subtract(const Amount& other) const;

    // Safe multiplication with overflow check
    std::optional<Amount> Multiply(uint64_t multiplier) const;

    // Safe division (returns nullopt if divisor is zero)
    std::optional<Amount> Divide(uint64_t divisor) const;

    // Check if amount is zero
    bool IsZero() const { return value_ == 0; }

    // Convert to string (for debugging/display)
    std::string ToString() const;

    // Serialize to bytes (8 bytes, little-endian)
    void Serialize(uint8_t* output) const;

    // Deserialize from bytes (8 bytes, little-endian)
    static Amount Deserialize(const uint8_t* input);

  private:
    uint64_t value_;
};

/**
 * Check if adding two amounts would overflow
 */
inline bool WouldAddOverflow(uint64_t a, uint64_t b) {
    return a > (UINT64_MAX - b);
}

/**
 * Check if subtracting would underflow
 */
inline bool WouldSubtractUnderflow(uint64_t a, uint64_t b) {
    return a < b;
}

/**
 * Check if multiplying would overflow
 */
inline bool WouldMultiplyOverflow(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0)
        return false;
    return a > (UINT64_MAX / b);
}

}  // namespace primitives
}  // namespace parthenon

#endif  // PARTHENON_PRIMITIVES_AMOUNT_H
