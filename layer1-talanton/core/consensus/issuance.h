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
     *
     * The issuance schedule is a Bitcoin-style geometric halving.
     * Achievable supply  =  initial_reward × HALVING_INTERVAL × 2
     * (continuous series with ratio ½; integer right-shift rounds down,
     *  so actual achievable is fractionally below this figure.)
     *
     *  Asset   reward/block    achievable              hard cap    gap
     *  ─────── ──────────── ────────────────────── ─────────────── ────────────────
     *  TALN     50 TALN       21 000 000 TALN          21 000 000   ~0 TALN  ✓
     *  DRM     238 000 DRM   99 960 000 000 DRM    100 000 000 000  40 000 000 DRM
     *  OBL     238 000 OBL   99 960 000 000 OBL    100 000 000 000  40 000 000 OBL
     *
     * DRACHMA and OBOLOS hard caps match the XRP maximum supply (100 billion).
     * All three layers use PoW mining; no staking is involved in issuance.
     */
    static constexpr uint64_t TALN_INITIAL_REWARD = 50ULL * primitives::AssetSupply::BASE_UNIT;
    static constexpr uint64_t DRM_INITIAL_REWARD = 238000ULL * primitives::AssetSupply::BASE_UNIT;
    static constexpr uint64_t OBL_INITIAL_REWARD = 238000ULL * primitives::AssetSupply::BASE_UNIT;
};

}  // namespace consensus
}  // namespace parthenon

#endif  // PARTHENON_CONSENSUS_ISSUANCE_H
