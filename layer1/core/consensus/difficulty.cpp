// ParthenonChain - Difficulty and Proof-of-Work Implementation
// Consensus-critical: Must be deterministic and match reference implementation

#include "difficulty.h"
#include <algorithm>
#include <cstring>

namespace parthenon {
namespace consensus {

std::array<uint8_t, 32> Difficulty::CompactToBits256(uint32_t compact) {
    std::array<uint8_t, 32> target{};
    
    // Extract exponent and mantissa
    uint32_t exponent = compact >> 24;
    uint32_t mantissa = compact & 0x00FFFFFF;
    
    // Check for negative or overflow (mantissa should not have high bit set in some contexts)
    // For our purposes, we'll allow it but handle correctly
    
    if (exponent == 0) {
        return target; // Zero target
    }
    
    // Exponent indicates the position of the most significant byte
    // Compact 0x1d00ffff means exponent=29, mantissa=0x00ffff
    // This represents: 0x00ffff * 256^(29-3) = 0x00ffff at byte position 26,27,28 (0-indexed, little-endian)
    // In big-endian representation: 0x00000000ffff00000000...
    // In little-endian byte array (index 0 is least significant):
    // bytes[28]=0xff, bytes[27]=0xff, bytes[26]=0x00
    
    if (exponent <= 3) {
        // Small exponent - mantissa is shifted down
        uint32_t shifted = mantissa >> (8 * (3 - exponent));
        target[0] = static_cast<uint8_t>(shifted);
        if (exponent >= 2) target[1] = static_cast<uint8_t>(shifted >> 8);
        if (exponent >= 3) target[2] = static_cast<uint8_t>(shifted >> 16);
    } else if (exponent <= 32) {
        // Place mantissa at the appropriate position (little-endian)
        size_t offset = static_cast<size_t>(exponent - 3); // Position of the least significant mantissa byte
        // Bounds check: ensure we don't write past array bounds
        if (offset + 2 < 32) {
            target[offset] = static_cast<uint8_t>(mantissa);
            target[offset + 1] = static_cast<uint8_t>(mantissa >> 8);
            target[offset + 2] = static_cast<uint8_t>(mantissa >> 16);
        } else if (offset + 1 < 32) {
            target[offset] = static_cast<uint8_t>(mantissa);
            target[offset + 1] = static_cast<uint8_t>(mantissa >> 8);
        } else if (offset < 32) {
            target[offset] = static_cast<uint8_t>(mantissa);
        }
        // else: offset too large, skip (invalid compact format)
    }
    // else: exponent too large, target remains zero (impossible difficulty)
    
    return target;
}

uint32_t Difficulty::Bits256ToCompact(const std::array<uint8_t, 32>& target) {
    // Find the most significant non-zero byte
    size_t exponent = 32;
    while (exponent > 0 && target[exponent - 1] == 0) {
        exponent--;
    }
    
    if (exponent == 0) {
        return 0; // All zeros
    }
    
    // Extract mantissa (3 bytes starting from most significant byte)
    uint32_t mantissa = 0;
    if (exponent >= 3) {
        mantissa = target[exponent - 1];
        mantissa = (mantissa << 8) | target[exponent - 2];
        mantissa = (mantissa << 8) | target[exponent - 3];
    } else if (exponent == 2) {
        mantissa = target[exponent - 1];
        mantissa = (mantissa << 8) | target[exponent - 2];
    } else { // exponent == 1
        mantissa = target[0];
    }
    
    // If high bit is set, we need to shift right and increment exponent
    if (mantissa & 0x00800000) {
        mantissa >>= 8;
        exponent++;
    }
    
    return (static_cast<uint32_t>(exponent) << 24) | mantissa;
}

int Difficulty::Compare256(
    const std::array<uint8_t, 32>& a,
    const std::array<uint8_t, 32>& b
) {
    // Compare from most significant byte (big-endian comparison)
    for (size_t i = 32; i > 0; i--) {
        if (a[i-1] < b[i-1]) return -1;
        if (a[i-1] > b[i-1]) return 1;
    }
    return 0;
}

bool Difficulty::CheckProofOfWork(
    const std::array<uint8_t, 32>& hash,
    uint32_t compact_bits
) {
    // Convert compact bits to 256-bit target
    auto target = CompactToBits256(compact_bits);
    
    // Hash must be <= target (numerically)
    return Compare256(hash, target) <= 0;
}

uint32_t Difficulty::GetInitialBits() {
    // Initial difficulty: 0x207fffff (very easy for testing)
    // This allows blocks to be mined quickly in tests
    // Production networks should use a harder initial difficulty
    return 0x207fffff;
}

uint32_t Difficulty::CalculateNextDifficulty(
    uint32_t current_bits,
    uint32_t time_span,
    uint32_t expected_time
) {
    // Apply timewarp protection: clamp actual timespan
    if (time_span < MIN_TIMESPAN) {
        time_span = MIN_TIMESPAN;
    }
    if (time_span > MAX_TIMESPAN) {
        time_span = MAX_TIMESPAN;
    }
    
    // Convert current target to 256-bit
    auto current_target = CompactToBits256(current_bits);
    
    // Calculate new target: current_target * time_span / expected_time
    // We need to do this carefully to avoid overflow
    // Using 64-bit arithmetic for intermediate results
    
    // First, find the most significant non-zero position
    size_t msb_pos = 31;
    while (msb_pos > 0 && current_target[msb_pos] == 0) {
        msb_pos--;
    }
    
    if (current_target[msb_pos] == 0) {
        // Current target is zero (shouldn't happen), return unchanged
        return current_bits;
    }
    
    // Extract significant bytes into a 64-bit number
    uint64_t target_val = 0;
    for (size_t i = 0; i <= std::min(msb_pos, static_cast<size_t>(7)); i++) {
        target_val |= static_cast<uint64_t>(current_target[msb_pos - i]) << (8 * i);
    }
    
    // Calculate new value
    target_val = (target_val * time_span) / expected_time;
    
    // Write back to target array
    std::array<uint8_t, 32> new_target{};
    for (size_t i = 0; i <= std::min(msb_pos, static_cast<size_t>(7)); i++) {
        new_target[msb_pos - i] = static_cast<uint8_t>(target_val >> (8 * i));
    }
    
    // Copy any remaining less significant bytes unchanged
    if (msb_pos > 7) {
        for (size_t i = 0; i < msb_pos - 7; i++) {
            new_target[i] = current_target[i];
        }
    }
    
    // Convert back to compact format
    return Bits256ToCompact(new_target);
}

} // namespace consensus
} // namespace parthenon
