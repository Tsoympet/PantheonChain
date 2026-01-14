// ParthenonChain - Difficulty and Proof-of-Work
// Consensus-critical: Difficulty target management and PoW validation

#ifndef PARTHENON_CONSENSUS_DIFFICULTY_H
#define PARTHENON_CONSENSUS_DIFFICULTY_H

#include <array>
#include <cstdint>

namespace parthenon {
namespace consensus {

/**
 * Difficulty represents a 256-bit target value
 * Blocks with hash <= target are valid
 */
class Difficulty {
  public:
    /**
     * Convert compact bits format to 256-bit target
     * Compact format: [exponent (1 byte)][mantissa (3 bytes)]
     */
    static std::array<uint8_t, 32> CompactToBits256(uint32_t compact);

    /**
     * Convert 256-bit target to compact bits format
     */
    static uint32_t Bits256ToCompact(const std::array<uint8_t, 32>& target);

    /**
     * Check if hash meets difficulty target
     * Returns true if hash <= target (numerically less than or equal)
     */
    static bool CheckProofOfWork(const std::array<uint8_t, 32>& hash, uint32_t compact_bits);

    /**
     * Get initial difficulty target (for genesis block)
     * This is a relatively easy target for testing and initial network bootstrap
     */
    static uint32_t GetInitialBits();

    /**
     * Calculate next difficulty target based on previous blocks
     * Implements Bitcoin-style difficulty adjustment with timewarp protection
     *
     * @param current_bits Current difficulty target (compact format)
     * @param time_span Actual time span of last difficulty period (in seconds)
     * @param expected_time Expected time span (target_spacing * interval)
     * @return New difficulty target (compact format)
     */
    static uint32_t CalculateNextDifficulty(uint32_t current_bits, uint32_t time_span,
                                            uint32_t expected_time);

    /**
     * Difficulty adjustment parameters
     */
    static constexpr uint32_t DIFFICULTY_ADJUSTMENT_INTERVAL = 2016;  // blocks
    static constexpr uint32_t TARGET_SPACING = 600;                   // seconds (10 minutes)
    static constexpr uint32_t TARGET_TIMESPAN =
        DIFFICULTY_ADJUSTMENT_INTERVAL * TARGET_SPACING;  // 2 weeks

    /**
     * Timewarp protection limits (prevent time manipulation attacks)
     */
    static constexpr uint32_t MIN_TIMESPAN = TARGET_TIMESPAN / 4;  // 3.5 days
    static constexpr uint32_t MAX_TIMESPAN = TARGET_TIMESPAN * 4;  // 8 weeks

  private:
    /**
     * Compare two 256-bit numbers (big-endian)
     * Returns: -1 if a < b, 0 if a == b, 1 if a > b
     */
    static int Compare256(const std::array<uint8_t, 32>& a, const std::array<uint8_t, 32>& b);
};

}  // namespace consensus
}  // namespace parthenon

#endif  // PARTHENON_CONSENSUS_DIFFICULTY_H
