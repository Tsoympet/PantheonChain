// ParthenonChain - Chain State Implementation
// Consensus-critical: Must enforce all consensus rules

#include "chainstate.h"

#include "consensus/difficulty.h"
#include "consensus/issuance.h"

namespace parthenon {
namespace chainstate {

std::map<primitives::AssetID, uint64_t>
ChainState::GetCoinbaseOutputs(const primitives::Transaction& coinbase) const {
    std::map<primitives::AssetID, uint64_t> outputs;

    // Sum outputs by asset
    for (const auto& output : coinbase.outputs) {
        auto asset = output.value.asset;
        auto amount = output.value.amount;

        if (outputs.find(asset) == outputs.end()) {
            outputs[asset] = 0;
        }
        outputs[asset] += amount;
    }

    return outputs;
}

bool ChainState::ValidateBlock(const primitives::Block& block) const {
    // Basic block structure validation
    if (!block.IsValid()) {
        return false;
    }

    // Must have at least coinbase transaction
    if (block.transactions.empty()) {
        return false;
    }

    const auto& coinbase = block.transactions[0];
    if (!coinbase.IsCoinbase()) {
        return false;
    }

    // Validate coinbase rewards
    auto coinbase_outputs = GetCoinbaseOutputs(coinbase);

    // Next block will be at height + 1
    uint64_t block_height = height_ + 1;

    // Check each asset's coinbase output
    for (const auto& [asset, amount] : coinbase_outputs) {
        // Verify reward doesn't exceed allowed amount
        if (!consensus::Issuance::IsValidBlockReward(block_height, asset, amount)) {
            return false;
        }

        // Verify adding this won't exceed supply cap (with overflow check)
        uint64_t current_supply = total_supply_.at(asset);
        uint64_t max_supply = primitives::AssetSupply::GetMaxSupply(asset);

        // Check for overflow
        if (current_supply + amount < current_supply) {
            return false;  // Overflow
        }

        uint64_t new_supply = current_supply + amount;
        if (new_supply > max_supply) {
            return false;
        }
    }

    // Verify PoW meets difficulty target
    auto hash = block.GetHash();
    if (!consensus::Difficulty::CheckProofOfWork(hash, block.header.bits)) {
        return false;
    }

    return true;
}

bool ChainState::ApplyBlock(const primitives::Block& block) {
    // Validate before applying
    if (!ValidateBlock(block)) {
        return false;
    }

    // Update height
    height_++;

    // Update total supplies from coinbase
    const auto& coinbase = block.transactions[0];
    auto coinbase_outputs = GetCoinbaseOutputs(coinbase);

    for (const auto& [asset, amount] : coinbase_outputs) {
        total_supply_[asset] += amount;
    }

    return true;
}

}  // namespace chainstate
}  // namespace parthenon
