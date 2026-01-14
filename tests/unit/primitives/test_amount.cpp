// ParthenonChain - Amount Tests
// Test overflow protection, arithmetic, and serialization

#include "primitives/amount.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace parthenon::primitives;

void TestAmountConstruction() {
    std::cout << "Test: Amount construction" << std::endl;

    Amount zero;
    assert(zero.GetValue() == 0);
    assert(zero.IsZero());

    Amount hundred(100);
    assert(hundred.GetValue() == 100);
    assert(!hundred.IsZero());

    Amount max(Amount::MAX_AMOUNT);
    assert(max.GetValue() == UINT64_MAX);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAmountComparison() {
    std::cout << "Test: Amount comparison" << std::endl;

    Amount a(100);
    Amount b(200);
    Amount c(100);

    assert(a == c);
    assert(a != b);
    assert(a < b);
    assert(b > a);
    assert(a <= c);
    assert(a >= c);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAmountAddition() {
    std::cout << "Test: Amount addition (safe)" << std::endl;

    // Normal addition
    Amount a(100);
    Amount b(50);
    auto result = a.Add(b);
    assert(result.has_value());
    assert(result->GetValue() == 150);

    // Addition with zero
    Amount zero(0);
    result = a.Add(zero);
    assert(result.has_value());
    assert(result->GetValue() == 100);

    // Overflow detection
    Amount max(UINT64_MAX);
    Amount one(1);
    result = max.Add(one);
    assert(!result.has_value());  // Should overflow

    // Near-max addition
    Amount near_max(UINT64_MAX - 10);
    Amount small(5);
    result = near_max.Add(small);
    assert(result.has_value());
    assert(result->GetValue() == UINT64_MAX - 5);

    // Exact overflow boundary
    result = near_max.Add(Amount(11));
    assert(!result.has_value());

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAmountSubtraction() {
    std::cout << "Test: Amount subtraction (safe)" << std::endl;

    // Normal subtraction
    Amount a(100);
    Amount b(30);
    auto result = a.Subtract(b);
    assert(result.has_value());
    assert(result->GetValue() == 70);

    // Subtract to zero
    Amount c(50);
    result = c.Subtract(c);
    assert(result.has_value());
    assert(result->IsZero());

    // Underflow detection
    Amount small(10);
    Amount large(100);
    result = small.Subtract(large);
    assert(!result.has_value());  // Should underflow

    // Subtract from zero
    Amount zero(0);
    result = zero.Subtract(Amount(1));
    assert(!result.has_value());

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAmountMultiplication() {
    std::cout << "Test: Amount multiplication (safe)" << std::endl;

    // Normal multiplication
    Amount a(100);
    auto result = a.Multiply(5);
    assert(result.has_value());
    assert(result->GetValue() == 500);

    // Multiply by zero
    result = a.Multiply(0);
    assert(result.has_value());
    assert(result->IsZero());

    // Multiply by one
    result = a.Multiply(1);
    assert(result.has_value());
    assert(result->GetValue() == 100);

    // Overflow detection
    Amount large(UINT64_MAX / 2);
    result = large.Multiply(3);
    assert(!result.has_value());  // Should overflow

    // Near-max multiplication
    Amount near_max(UINT64_MAX / 10);
    result = near_max.Multiply(10);
    assert(result.has_value());

    result = near_max.Multiply(11);
    assert(!result.has_value());

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAmountDivision() {
    std::cout << "Test: Amount division (safe)" << std::endl;

    // Normal division
    Amount a(100);
    auto result = a.Divide(5);
    assert(result.has_value());
    assert(result->GetValue() == 20);

    // Division by one
    result = a.Divide(1);
    assert(result.has_value());
    assert(result->GetValue() == 100);

    // Division by zero
    result = a.Divide(0);
    assert(!result.has_value());

    // Integer division
    Amount b(10);
    result = b.Divide(3);
    assert(result.has_value());
    assert(result->GetValue() == 3);  // Floor division

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAmountSerialization() {
    std::cout << "Test: Amount serialization" << std::endl;

    // Test zero
    Amount zero(0);
    uint8_t buffer[8];
    zero.Serialize(buffer);
    Amount deserialized = Amount::Deserialize(buffer);
    assert(deserialized == zero);

    // Test small number
    Amount small(12345);
    small.Serialize(buffer);
    deserialized = Amount::Deserialize(buffer);
    assert(deserialized == small);

    // Test large number
    Amount large(0xFEDCBA9876543210ULL);
    large.Serialize(buffer);
    deserialized = Amount::Deserialize(buffer);
    assert(deserialized == large);

    // Test max value
    Amount max(UINT64_MAX);
    max.Serialize(buffer);
    deserialized = Amount::Deserialize(buffer);
    assert(deserialized == max);

    // Verify little-endian
    Amount test(0x0102030405060708ULL);
    test.Serialize(buffer);
    assert(buffer[0] == 0x08);
    assert(buffer[1] == 0x07);
    assert(buffer[2] == 0x06);
    assert(buffer[3] == 0x05);
    assert(buffer[4] == 0x04);
    assert(buffer[5] == 0x03);
    assert(buffer[6] == 0x02);
    assert(buffer[7] == 0x01);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestOverflowHelpers() {
    std::cout << "Test: Overflow helper functions" << std::endl;

    // Add overflow
    assert(!WouldAddOverflow(100, 200));
    assert(WouldAddOverflow(UINT64_MAX, 1));
    assert(WouldAddOverflow(UINT64_MAX - 5, 10));
    assert(!WouldAddOverflow(UINT64_MAX - 10, 10));

    // Subtract underflow
    assert(!WouldSubtractUnderflow(100, 50));
    assert(WouldSubtractUnderflow(50, 100));
    assert(WouldSubtractUnderflow(0, 1));

    // Multiply overflow
    assert(!WouldMultiplyOverflow(100, 200));
    assert(WouldMultiplyOverflow(UINT64_MAX, 2));
    assert(WouldMultiplyOverflow(UINT64_MAX / 2, 3));
    assert(!WouldMultiplyOverflow(0, UINT64_MAX));
    assert(!WouldMultiplyOverflow(UINT64_MAX, 0));

    std::cout << "  ✓ Passed" << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "ParthenonChain Amount Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;

    try {
        TestAmountConstruction();
        TestAmountComparison();
        TestAmountAddition();
        TestAmountSubtraction();
        TestAmountMultiplication();
        TestAmountDivision();
        TestAmountSerialization();
        TestOverflowHelpers();

        std::cout << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "All Amount tests passed! ✓" << std::endl;
        std::cout << "=====================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
