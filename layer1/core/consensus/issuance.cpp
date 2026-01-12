// ParthenonChain - Issuance Schedule Implementation
// Consensus-critical: Must be deterministic and never exceed supply caps

#include "issuance.h"

namespace parthenon {
namespace consensus {

uint64_t Issuance::GetInitialReward(primitives::AssetID asset) {
    switch (asset) {
        case primitives::AssetID::TALANTON:
            return TALN_INITIAL_REWARD;
        case primitives::AssetID::DRACHMA:
            return DRM_INITIAL_REWARD;
        case primitives::AssetID::OBOLOS:
            return OBL_INITIAL_REWARD;
        default:
            return 0;
    }
}

uint64_t Issuance::GetBlockReward(uint64_t height, primitives::AssetID asset) {
    // Calculate halving epoch
    uint64_t halvings = height / HALVING_INTERVAL;
    
    // After 64 halvings, reward is zero (2^64 would overflow)
    if (halvings >= 64) {
        return 0;
    }
    
    // Get initial reward for this asset
    uint64_t reward = GetInitialReward(asset);
    
    // Apply halvings by right-shifting (divide by 2^halvings)
    reward >>= halvings;
    
    return reward;
}

uint64_t Issuance::CalculateSupplyAtHeight(uint64_t height, primitives::AssetID asset) {
    uint64_t total_supply = 0;
    
    // Calculate supply for each halving epoch up to current height
    uint64_t remaining_height = height;
    uint64_t halvings = 0;
    
    while (remaining_height > 0 && halvings < 64) {
        // Number of blocks in this epoch
        uint64_t blocks_in_epoch = (remaining_height > HALVING_INTERVAL) 
            ? HALVING_INTERVAL 
            : remaining_height;
        
        // Reward for this epoch
        uint64_t reward = GetInitialReward(asset) >> halvings;
        
        // Add to total supply (with overflow check)
        uint64_t epoch_supply = blocks_in_epoch * reward;
        if (epoch_supply / blocks_in_epoch != reward) {
            // Overflow detected, return max supply
            return primitives::AssetSupply::GetMaxSupply(asset);
        }
        
        if (total_supply + epoch_supply < total_supply) {
            // Overflow detected, return max supply
            return primitives::AssetSupply::GetMaxSupply(asset);
        }
        
        total_supply += epoch_supply;
        remaining_height -= blocks_in_epoch;
        halvings++;
    }
    
    return total_supply;
}

bool Issuance::IsValidBlockReward(
    uint64_t height,
    primitives::AssetID asset,
    uint64_t amount
) {
    // Calculate expected reward
    uint64_t expected_reward = GetBlockReward(height, asset);
    
    // Amount must not exceed expected reward
    if (amount > expected_reward) {
        return false;
    }
    
    // Verify that total supply at this height won't exceed cap
    uint64_t projected_supply = CalculateSupplyAtHeight(height + 1, asset);
    if (projected_supply > primitives::AssetSupply::GetMaxSupply(asset)) {
        return false;
    }
    
    return true;
}

} // namespace consensus
} // namespace parthenon
