// ParthenonChain - Chain State
// Consensus-critical: Blockchain state tracking and validation

#ifndef PARTHENON_CHAINSTATE_CHAINSTATE_H
#define PARTHENON_CHAINSTATE_CHAINSTATE_H

#include "primitives/asset.h"
#include "primitives/block.h"

#include <cstdint>
#include <map>

namespace parthenon {
namespace chainstate {

/**
 * ChainState tracks the current state of the blockchain
 * including height, total supply per asset, and validation state
 */
class ChainState {
  public:
    ChainState() : height_(0), tip_hash_{}, total_supply_() {
        // Initialize all asset supplies to zero
        total_supply_[primitives::AssetID::TALANTON] = 0;
        total_supply_[primitives::AssetID::DRACHMA] = 0;
        total_supply_[primitives::AssetID::OBOLOS] = 0;
    }

    /**
     * Get current blockchain height
     */
    uint64_t GetHeight() const { return height_; }

    /**
     * Get current tip hash (zero hash if no blocks)
     */
    std::array<uint8_t, 32> GetTipHash() const { return tip_hash_; }

    /**
     * Get total supply for an asset
     */
    uint64_t GetTotalSupply(primitives::AssetID asset) const {
        auto it = total_supply_.find(asset);
        if (it != total_supply_.end()) {
            return it->second;
        }
        return 0;
    }

    /**
     * Apply a block to the chain state
     * Updates height and total supplies
     *
     * @param block Block to apply
     * @return true if block was successfully applied
     */
    bool ApplyBlock(const primitives::Block& block);

    /**
     * Validate that a block can be applied to current state
     * Checks:
     * - Block structure is valid
     * - Difficulty target is correct
     * - Coinbase reward is valid
     * - Total supplies won't exceed caps
     *
     * @param block Block to validate
     * @return true if block is valid for current state
     */
    bool ValidateBlock(const primitives::Block& block) const;

    /**
     * Reset chain state to genesis
     */
    void Reset() {
        height_ = 0;
        tip_hash_.fill(0);
        total_supply_[primitives::AssetID::TALANTON] = 0;
        total_supply_[primitives::AssetID::DRACHMA] = 0;
        total_supply_[primitives::AssetID::OBOLOS] = 0;
    }

  private:
    uint64_t height_;
    std::array<uint8_t, 32> tip_hash_;
    std::map<primitives::AssetID, uint64_t> total_supply_;

    /**
     * Calculate total coinbase outputs by asset
     */
    std::map<primitives::AssetID, uint64_t>
    GetCoinbaseOutputs(const primitives::Transaction& coinbase) const;
};

}  // namespace chainstate
}  // namespace parthenon

#endif  // PARTHENON_CHAINSTATE_CHAINSTATE_H
