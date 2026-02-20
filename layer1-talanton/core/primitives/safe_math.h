// ParthenonChain - Safe Math Utilities
// Overflow-safe integer arithmetic for financial calculations

#ifndef PARTHENON_PRIMITIVES_SAFE_MATH_H
#define PARTHENON_PRIMITIVES_SAFE_MATH_H

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

namespace parthenon {
namespace primitives {

/**
 * SafeMath provides overflow-safe arithmetic operations
 * Essential for financial calculations to prevent integer overflow vulnerabilities
 */
class SafeMath {
  public:
    /**
     * Safe addition with overflow detection
     * @return std::nullopt if overflow would occur
     */
    static std::optional<uint64_t> Add(uint64_t a, uint64_t b) {
        if (a > std::numeric_limits<uint64_t>::max() - b) {
            return std::nullopt;  // Overflow would occur
        }
        return a + b;
    }

    /**
     * Safe subtraction with underflow detection
     * @return std::nullopt if underflow would occur
     */
    static std::optional<uint64_t> Sub(uint64_t a, uint64_t b) {
        if (a < b) {
            return std::nullopt;  // Underflow would occur
        }
        return a - b;
    }

    /**
     * Safe multiplication with overflow detection
     * @return std::nullopt if overflow would occur
     */
    static std::optional<uint64_t> Mul(uint64_t a, uint64_t b) {
        if (a == 0 || b == 0) {
            return 0;
        }
        if (a > std::numeric_limits<uint64_t>::max() / b) {
            return std::nullopt;  // Overflow would occur
        }
        return a * b;
    }

    /**
     * Safe division with divide-by-zero detection
     * @return std::nullopt if division by zero
     */
    static std::optional<uint64_t> Div(uint64_t a, uint64_t b) {
        if (b == 0) {
            return std::nullopt;  // Division by zero
        }
        return a / b;
    }

    /**
     * Safe modulo with divide-by-zero detection
     * @return std::nullopt if division by zero
     */
    static std::optional<uint64_t> Mod(uint64_t a, uint64_t b) {
        if (b == 0) {
            return std::nullopt;  // Division by zero
        }
        return a % b;
    }

    /**
     * Safe percentage calculation
     * @param amount Base amount
     * @param percentage Percentage (0-10000 for basis points, or 0-100 for percent)
     * @param divisor Divisor (10000 for basis points, 100 for percent)
     * @return std::nullopt if overflow would occur
     */
    static std::optional<uint64_t> Percentage(uint64_t amount, uint64_t percentage,
                                              uint64_t divisor = 100) {
        auto mul_result = Mul(amount, percentage);
        if (!mul_result) {
            return std::nullopt;
        }
        return Div(*mul_result, divisor);
    }

    /**
     * Checked add - throws on overflow
     */
    static uint64_t CheckedAdd(uint64_t a, uint64_t b) {
        auto result = Add(a, b);
        if (!result) {
            throw std::overflow_error("Integer overflow in addition");
        }
        return *result;
    }

    /**
     * Checked subtract - throws on underflow
     */
    static uint64_t CheckedSub(uint64_t a, uint64_t b) {
        auto result = Sub(a, b);
        if (!result) {
            throw std::underflow_error("Integer underflow in subtraction");
        }
        return *result;
    }

    /**
     * Checked multiply - throws on overflow
     */
    static uint64_t CheckedMul(uint64_t a, uint64_t b) {
        auto result = Mul(a, b);
        if (!result) {
            throw std::overflow_error("Integer overflow in multiplication");
        }
        return *result;
    }

    /**
     * Checked divide - throws on division by zero
     */
    static uint64_t CheckedDiv(uint64_t a, uint64_t b) {
        auto result = Div(a, b);
        if (!result) {
            throw std::domain_error("Division by zero");
        }
        return *result;
    }
};

}  // namespace primitives
}  // namespace parthenon

#endif  // PARTHENON_PRIMITIVES_SAFE_MATH_H
