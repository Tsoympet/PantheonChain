// ParthenonChain - Difficulty Tests
// Test difficulty conversion, PoW validation, and adjustment algorithm

#include "consensus/difficulty.h"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace parthenon::consensus;

void PrintHash(const std::array<uint8_t, 32>& hash) {
    for (int i = 31; i >= 0; i--) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(hash[i]);
    }
    std::cout << std::dec << std::endl;
}

void TestCompactConversion() {
    std::cout << "Test: Compact bits conversion" << std::endl;
    
    // Test 0x1d00ffff (Bitcoin difficulty example)
    // Exponent 0x1d = 29, Mantissa 0x00ffff
    // Value = 0x00ffff * 256^(29-3) = 0x00ffff * 256^26
    // This places mantissa bytes at positions 26, 27, 28 (little-endian)
    uint32_t compact = 0x1d00ffff;
    auto target = Difficulty::CompactToBits256(compact);
    
    // In little-endian byte array: target[26]=0xff, target[27]=0xff, target[28]=0x00
    assert(target[26] == 0xff);
    assert(target[27] == 0xff);
    assert(target[28] == 0x00);
    
    // Convert back
    uint32_t compact2 = Difficulty::Bits256ToCompact(target);
    assert(compact2 == compact);
    
    std::cout << "  ✓ Passed (round-trip conversion)" << std::endl;
}

void TestCompactConversionExamples() {
    std::cout << "Test: Various compact formats" << std::endl;
    
    // Test several compact values
    uint32_t compacts[] = {
        0x1d00ffff,  // Bitcoin initial
        0x1b0404cb,  // Higher difficulty
        0x1a05db8b,  // Even higher difficulty
        0x03123456,  // Very low exponent
    };
    
    for (uint32_t compact : compacts) {
        auto target = Difficulty::CompactToBits256(compact);
        uint32_t compact2 = Difficulty::Bits256ToCompact(target);
        
        // Should round-trip (may differ slightly due to normalization)
        auto target2 = Difficulty::CompactToBits256(compact2);
        assert(target == target2);
    }
    
    std::cout << "  ✓ Passed (multiple values)" << std::endl;
}

void TestProofOfWork() {
    std::cout << "Test: Proof of work validation" << std::endl;
    
    // Create a hash that should pass easy difficulty (0x1d00ffff)
    // Target is: bytes[26]=0xff, [27]=0xff, [28]=0x00, rest are 0
    // Which is: 0x0000ffff000000... in big-endian
    // Hash needs to be <= target, so let's use 0x0000ff0000...
    std::array<uint8_t, 32> easy_hash{};
    easy_hash[27] = 0xff;  // Less than target at this position
    easy_hash[26] = 0x00;
    
    uint32_t easy_bits = 0x1d00ffff;
    assert(Difficulty::CheckProofOfWork(easy_hash, easy_bits));
    
    // Create a hash that should fail (all 0xff)
    std::array<uint8_t, 32> hard_hash;
    hard_hash.fill(0xff);
    
    assert(!Difficulty::CheckProofOfWork(hard_hash, easy_bits));
    
    std::cout << "  ✓ Passed (validation logic)" << std::endl;
}

void TestDifficultyAdjustment() {
    std::cout << "Test: Difficulty adjustment algorithm" << std::endl;
    
    uint32_t current_bits = 0x1d00ffff;
    uint32_t expected_time = Difficulty::TARGET_TIMESPAN;
    
    // Test 1: Actual time equals expected (no change)
    uint32_t new_bits = Difficulty::CalculateNextDifficulty(
        current_bits, expected_time, expected_time
    );
    assert(new_bits == current_bits);
    
    // Test 2: Blocks found too quickly (double difficulty)
    uint32_t half_time = expected_time / 2;
    new_bits = Difficulty::CalculateNextDifficulty(
        current_bits, half_time, expected_time
    );
    // Should increase difficulty (lower target, higher compact exponent or lower mantissa)
    assert(new_bits != current_bits);
    
    // Test 3: Blocks found too slowly (halve difficulty)
    uint32_t double_time = expected_time * 2;
    new_bits = Difficulty::CalculateNextDifficulty(
        current_bits, double_time, expected_time
    );
    // Should decrease difficulty (higher target)
    assert(new_bits != current_bits);
    
    std::cout << "  ✓ Passed (adjustment logic)" << std::endl;
}

void TestTimewarpProtection() {
    std::cout << "Test: Timewarp protection" << std::endl;
    
    uint32_t current_bits = 0x1d00ffff;
    uint32_t expected_time = Difficulty::TARGET_TIMESPAN;
    
    // Test extreme values are clamped
    uint32_t very_short = 100; // Much less than MIN_TIMESPAN
    uint32_t new_bits_short = Difficulty::CalculateNextDifficulty(
        current_bits, very_short, expected_time
    );
    
    // Should be clamped to MIN_TIMESPAN
    uint32_t new_bits_min = Difficulty::CalculateNextDifficulty(
        current_bits, Difficulty::MIN_TIMESPAN, expected_time
    );
    assert(new_bits_short == new_bits_min);
    
    // Test max timespan
    uint32_t very_long = 100000000; // Much more than MAX_TIMESPAN
    uint32_t new_bits_long = Difficulty::CalculateNextDifficulty(
        current_bits, very_long, expected_time
    );
    
    uint32_t new_bits_max = Difficulty::CalculateNextDifficulty(
        current_bits, Difficulty::MAX_TIMESPAN, expected_time
    );
    assert(new_bits_long == new_bits_max);
    
    std::cout << "  ✓ Passed (clamping works)" << std::endl;
}

void TestInitialDifficulty() {
    std::cout << "Test: Initial difficulty" << std::endl;
    
    uint32_t initial = Difficulty::GetInitialBits();
    assert(initial == 0x207fffff);
    
    // Verify it's a reasonable target (very easy for testing)
    auto target = Difficulty::CompactToBits256(initial);
    // 0x207fffff = exponent 32, mantissa 0x7fffff
    // This is maximum difficulty (easiest) - almost any hash will work
    assert(target[29] == 0xff);
    assert(target[30] == 0xff);
    assert(target[31] == 0x7f);
    
    std::cout << "  ✓ Passed (initial target set)" << std::endl;
}

int main() {
    std::cout << "=== Difficulty Tests ===" << std::endl;
    
    TestCompactConversion();
    TestCompactConversionExamples();
    TestProofOfWork();
    TestDifficultyAdjustment();
    TestTimewarpProtection();
    TestInitialDifficulty();
    
    std::cout << "\n✓ All difficulty tests passed!" << std::endl;
    return 0;
}
