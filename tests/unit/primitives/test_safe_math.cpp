// ParthenonChain - SafeMath Tests
// Test overflow-safe arithmetic operations

#include "primitives/safe_math.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace parthenon::primitives;

void test_safe_addition() {
    std::cout << "Testing SafeMath::Add..." << std::endl;

    // Normal addition
    auto result1 = SafeMath::Add(100, 200);
    assert(result1.has_value());
    assert(*result1 == 300);

    // Overflow detection
    auto result2 = SafeMath::Add(UINT64_MAX, 1);
    assert(!result2.has_value());

    auto result3 = SafeMath::Add(UINT64_MAX - 10, 11);
    assert(!result3.has_value());

    // Edge case: adding zero
    auto result4 = SafeMath::Add(UINT64_MAX, 0);
    assert(result4.has_value());
    assert(*result4 == UINT64_MAX);

    std::cout << "  ✓ Addition tests passed" << std::endl;
}

void test_safe_subtraction() {
    std::cout << "Testing SafeMath::Sub..." << std::endl;

    // Normal subtraction
    auto result1 = SafeMath::Sub(200, 100);
    assert(result1.has_value());
    assert(*result1 == 100);

    // Underflow detection
    auto result2 = SafeMath::Sub(100, 200);
    assert(!result2.has_value());

    auto result3 = SafeMath::Sub(0, 1);
    assert(!result3.has_value());

    // Edge case: subtracting zero
    auto result4 = SafeMath::Sub(100, 0);
    assert(result4.has_value());
    assert(*result4 == 100);

    std::cout << "  ✓ Subtraction tests passed" << std::endl;
}

void test_safe_multiplication() {
    std::cout << "Testing SafeMath::Mul..." << std::endl;

    // Normal multiplication
    auto result1 = SafeMath::Mul(100, 200);
    assert(result1.has_value());
    assert(*result1 == 20000);

    // Overflow detection
    auto result2 = SafeMath::Mul(UINT64_MAX, 2);
    assert(!result2.has_value());

    auto result3 = SafeMath::Mul(UINT64_MAX / 2 + 1, 2);
    assert(!result3.has_value());

    // Edge case: multiply by zero
    auto result4 = SafeMath::Mul(UINT64_MAX, 0);
    assert(result4.has_value());
    assert(*result4 == 0);

    // Edge case: multiply by one
    auto result5 = SafeMath::Mul(12345, 1);
    assert(result5.has_value());
    assert(*result5 == 12345);

    std::cout << "  ✓ Multiplication tests passed" << std::endl;
}

void test_safe_division() {
    std::cout << "Testing SafeMath::Div..." << std::endl;

    // Normal division
    auto result1 = SafeMath::Div(200, 100);
    assert(result1.has_value());
    assert(*result1 == 2);

    // Division by zero detection
    auto result2 = SafeMath::Div(100, 0);
    assert(!result2.has_value());

    // Edge case: divide zero
    auto result3 = SafeMath::Div(0, 100);
    assert(result3.has_value());
    assert(*result3 == 0);

    // Edge case: divide by one
    auto result4 = SafeMath::Div(12345, 1);
    assert(result4.has_value());
    assert(*result4 == 12345);

    std::cout << "  ✓ Division tests passed" << std::endl;
}

void test_safe_percentage() {
    std::cout << "Testing SafeMath::Percentage..." << std::endl;

    // Normal percentage (50% of 1000)
    auto result1 = SafeMath::Percentage(1000, 50);
    assert(result1.has_value());
    assert(*result1 == 500);

    // Basis points (0.3% of 10000)
    auto result2 = SafeMath::Percentage(10000, 30, 10000);
    assert(result2.has_value());
    assert(*result2 == 30);

    // Overflow detection
    auto result3 = SafeMath::Percentage(UINT64_MAX, 100);
    assert(!result3.has_value());

    // Zero percentage
    auto result4 = SafeMath::Percentage(1000, 0);
    assert(result4.has_value());
    assert(*result4 == 0);

    std::cout << "  ✓ Percentage tests passed" << std::endl;
}

void test_checked_operations() {
    std::cout << "Testing checked operations..." << std::endl;

    // CheckedAdd - normal case
    try {
        assert(SafeMath::CheckedAdd(100, 200) == 300);
    } catch (...) {
        assert(false && "Should not throw");
    }

    // CheckedAdd - overflow
    try {
        SafeMath::CheckedAdd(UINT64_MAX, 1);
        assert(false && "Should have thrown");
    } catch (const std::overflow_error&) {
        // Expected
    }

    // CheckedSub - normal case
    try {
        assert(SafeMath::CheckedSub(200, 100) == 100);
    } catch (...) {
        assert(false && "Should not throw");
    }

    // CheckedSub - underflow
    try {
        SafeMath::CheckedSub(100, 200);
        assert(false && "Should have thrown");
    } catch (const std::underflow_error&) {
        // Expected
    }

    // CheckedMul - normal case
    try {
        assert(SafeMath::CheckedMul(100, 200) == 20000);
    } catch (...) {
        assert(false && "Should not throw");
    }

    // CheckedMul - overflow
    try {
        SafeMath::CheckedMul(UINT64_MAX, 2);
        assert(false && "Should have thrown");
    } catch (const std::overflow_error&) {
        // Expected
    }

    // CheckedDiv - normal case
    try {
        const uint64_t result = SafeMath::CheckedDiv(200, 100);
        if (result != 2) {
            throw std::runtime_error("CheckedDiv returned unexpected value");
        }
    } catch (...) {
        assert(false && "Should not throw");
    }

    // CheckedDiv - division by zero
    try {
        SafeMath::CheckedDiv(100, 0);
        assert(false && "Should have thrown");
    } catch (const std::domain_error&) {
        // Expected
    }

    std::cout << "  ✓ Checked operations tests passed" << std::endl;
}

int main() {
    std::cout << "=== SafeMath Tests ===" << std::endl;
    std::cout << std::endl;

    try {
        test_safe_addition();
        test_safe_subtraction();
        test_safe_multiplication();
        test_safe_division();
        test_safe_percentage();
        test_checked_operations();

        std::cout << std::endl;
        std::cout << "✓ All SafeMath tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
