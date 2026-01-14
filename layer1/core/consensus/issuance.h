// ParthenonChain - Issuance Schedule
// Consensus-critical: Block reward calculation for all three assets

#ifndef PARTHENON_CONSENSUS_ISSUANCE_H
#define PARTHENON_CONSENSUS_ISSUANCE_H

#include "primitives/asset.h"

#include <cstdint>

namespace parthenon {
namespace consensus {

/**
 * Issuance manages the creation of new coins through block rewards
 * Three parallel issuance schedules for TALN, DRM, and OBL
 */
class Issuance {
  public:
    /**
     * Calculate block reward for each asset at given height
     * Uses halving schedule similar to Bitcoin
     *
     * @param height Block height
     * @param asset Asset to calculate reward for
     * @return Reward amount in base units
     */
    static uint64_t GetBlockReward(uint64_t height, primitives::AssetID asset);

    /**
     * Get initial block reward for each asset
     */
    static uint64_t GetInitialReward(primitives::AssetID asset);

    /**
     * Get halving interval (same for all assets)
     * Bitcoin uses 210,000 blocks (approximately 4 years)
     */
    static constexpr uint64_t HALVING_INTERVAL = 210000;

    /**
     * Calculate total supply that will exist at given height
     * This is used to verify supply caps are never exceeded
     *
     * @param height Block height
     * @param asset Asset to calculate supply for
     * @return Total supply in base units
     */
    static uint64_t CalculateSupplyAtHeight(uint64_t height, primitives::AssetID asset);

    /**
     * Verify that block reward respects issuance schedule
     *
     * @param height Block height
     * @param asset Asset being rewarded
     * @param amount Claimed reward amount
     * @return true if reward is valid
     */
    static bool IsValidBlockReward(uint64_t height, primitives::AssetID asset, uint64_t amount);

  private:
    /**
     * Initial block rewards (in base units)
     * These are chosen to reach the target max supplies with the halving schedule
     * Total supply = initial_reward * HALVING_INTERVAL * 2
     * - TALN: 50 * 210000 * 2 = 21,000,000
     * - DRM:  97 * 210000 * 2 = 40,740,000 (slightly under 41M cap)
     * - OBL: 145 * 210000 * 2 = 60,900,000 (slightly under 61M cap)
     */
    static constexpr uint64_t TALN_INITIAL_REWARD = 50ULL * primitives::AssetSupply::BASE_UNIT;
    static constexpr uint64_t DRM_INITIAL_REWARD = 97ULL * primitives::AssetSupply::BASE_UNIT;
    static constexpr uint64_t OBL_INITIAL_REWARD = 145ULL * primitives::AssetSupply::BASE_UNIT;
};

}  // namespace consensus
}  // namespace parthenon

#endif  // PARTHENON_CONSENSUS_ISSUANCE_H
