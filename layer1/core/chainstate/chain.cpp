// ParthenonChain - Chain Implementation
// Consensus-critical: Must be deterministic

#include "chain.h"
#include "consensus/issuance.h"
#include "consensus/difficulty.h"
#include <set>

namespace parthenon {
namespace chainstate {

Chain::Chain() : height_(0), tip_hash_{} {
    total_supply_[primitives::AssetID::TALANTON] = 0;
    total_supply_[primitives::AssetID::DRACHMA] = 0;
    total_supply_[primitives::AssetID::OBOLOS] = 0;
}

bool Chain::ValidateTransaction(const primitives::Transaction& tx, uint32_t height) const {
    // Coinbase transactions are validated separately
    if (tx.IsCoinbase()) {
        return true;
    }
    
    // Check that all inputs exist in UTXO set
    for (const auto& input : tx.inputs) {
        auto coin = utxo_set_.GetCoin(input.prevout);
        if (!coin) {
            return false; // Input does not exist
        }
        
        // Check if coin is spendable (coinbase maturity)
        if (!coin->IsSpendable(height)) {
            return false; // Coinbase not yet mature
        }
    }
    
    // Check no duplicate inputs
    std::set<primitives::OutPoint> seen_inputs;
    for (const auto& input : tx.inputs) {
        if (seen_inputs.count(input.prevout) > 0) {
            return false; // Duplicate input
        }
        seen_inputs.insert(input.prevout);
    }
    
    // Asset conservation: inputs == outputs per asset
    std::map<primitives::AssetID, uint64_t> input_amounts;
    std::map<primitives::AssetID, uint64_t> output_amounts;
    
    for (const auto& input : tx.inputs) {
        auto coin = utxo_set_.GetCoin(input.prevout);
        if (coin) {
            auto asset = coin->output.value.asset;
            input_amounts[asset] += coin->output.value.amount;
        }
    }
    
    for (const auto& output : tx.outputs) {
        auto asset = output.value.asset;
        output_amounts[asset] += output.value.amount;
    }
    
    // Check conservation for each asset
    for (const auto& [asset, amount] : output_amounts) {
        if (input_amounts[asset] < amount) {
            return false; // Cannot create assets from thin air
        }
    }
    
    return true;
}

void Chain::UpdateSupply(const primitives::Transaction& coinbase, bool connect) {
    if (!coinbase.IsCoinbase()) {
        return;
    }
    
    // Sum coinbase outputs by asset
    std::map<primitives::AssetID, uint64_t> amounts;
    for (const auto& output : coinbase.outputs) {
        amounts[output.value.asset] += output.value.amount;
    }
    
    // Update total supply
    for (const auto& [asset, amount] : amounts) {
        if (connect) {
            total_supply_[asset] += amount;
        } else {
            total_supply_[asset] -= amount;
        }
    }
}

bool Chain::ConnectBlock(const primitives::Block& block, BlockUndo& undo) {
    // Validate block structure
    if (!block.IsValid()) {
        return false;
    }
    
    // Verify PoW
    if (!consensus::Difficulty::CheckProofOfWork(block.GetHash(), block.header.bits)) {
        return false;
    }
    
    // Must have coinbase
    if (block.transactions.empty() || !block.transactions[0].IsCoinbase()) {
        return false;
    }
    
    uint32_t block_height = height_ + 1;
    
    // Validate coinbase rewards
    const auto& coinbase = block.transactions[0];
    std::map<primitives::AssetID, uint64_t> coinbase_amounts;
    for (const auto& output : coinbase.outputs) {
        coinbase_amounts[output.value.asset] += output.value.amount;
    }
    
    for (const auto& [asset, amount] : coinbase_amounts) {
        if (!consensus::Issuance::IsValidBlockReward(block_height, asset, amount)) {
            return false;
        }
        
        // Check supply cap
        uint64_t new_supply = total_supply_[asset] + amount;
        if (new_supply < total_supply_[asset]) {
            return false; // Overflow
        }
        if (new_supply > primitives::AssetSupply::GetMaxSupply(asset)) {
            return false; // Exceeds cap
        }
    }
    
    // Process transactions
    for (size_t i = 0; i < block.transactions.size(); i++) {
        const auto& tx = block.transactions[i];
        
        if (i == 0) {
            // Coinbase: add outputs to UTXO set
            auto txid = tx.GetTxID();
            for (uint32_t vout = 0; vout < tx.outputs.size(); vout++) {
                primitives::OutPoint outpoint(txid, vout);
                Coin coin(tx.outputs[vout], block_height, true);
                utxo_set_.AddCoin(outpoint, coin);
            }
        } else {
            // Regular transaction: validate and update UTXO set
            if (!ValidateTransaction(tx, block_height)) {
                return false;
            }
            
            // Collect spent coins for undo
            std::vector<Coin> tx_undo;
            for (const auto& input : tx.inputs) {
                auto coin = utxo_set_.GetCoin(input.prevout);
                if (coin) {
                    tx_undo.push_back(*coin);
                    utxo_set_.SpendCoin(input.prevout);
                }
            }
            undo.AddTxUndo(tx_undo);
            
            // Add new outputs
            auto txid = tx.GetTxID();
            for (uint32_t vout = 0; vout < tx.outputs.size(); vout++) {
                primitives::OutPoint outpoint(txid, vout);
                Coin coin(tx.outputs[vout], block_height, false);
                utxo_set_.AddCoin(outpoint, coin);
            }
        }
    }
    
    // Update chain state
    height_ = block_height;
    tip_hash_ = block.GetHash();
    UpdateSupply(coinbase, true);
    
    // Add to block index
    uint64_t chain_work = 1; // Default for genesis
    if (height_ > 1) {
        // Get previous block's chain work
        auto prev_index = block_index_.find(block.header.prev_block_hash);
        if (prev_index != block_index_.end()) {
            chain_work = prev_index->second.chain_work + 1;
        }
        // If prev block not found, still use 1 (shouldn't happen in valid chain)
    }
    block_index_[tip_hash_] = BlockIndex(block.header, height_, chain_work);
    
    return true;
}

bool Chain::DisconnectBlock(const primitives::Block& block, const BlockUndo& undo) {
    if (height_ == 0) {
        return false; // Cannot disconnect genesis
    }
    
    // Verify this is the tip block
    if (block.GetHash() != tip_hash_) {
        return false; // Can only disconnect tip
    }
    
    uint32_t block_height = height_;
    
    // Process transactions in reverse order
    size_t undo_index = undo.tx_undo.size();
    for (size_t i = block.transactions.size(); i-- > 0;) {
        const auto& tx = block.transactions[i];
        auto txid = tx.GetTxID();
        
        if (i == 0) {
            // Coinbase: remove outputs from UTXO set
            for (uint32_t vout = 0; vout < tx.outputs.size(); vout++) {
                primitives::OutPoint outpoint(txid, vout);
                utxo_set_.SpendCoin(outpoint);
            }
        } else {
            // Regular transaction: restore spent coins
            // Remove outputs created by this transaction
            for (uint32_t vout = 0; vout < tx.outputs.size(); vout++) {
                primitives::OutPoint outpoint(txid, vout);
                utxo_set_.SpendCoin(outpoint);
            }
            
            // Restore spent inputs
            undo_index--;
            if (undo_index >= undo.tx_undo.size()) {
                return false; // Invalid undo data
            }
            
            const auto& tx_undo = undo.tx_undo[undo_index];
            if (tx_undo.size() != tx.inputs.size()) {
                return false; // Mismatched undo data
            }
            
            for (size_t j = 0; j < tx.inputs.size(); j++) {
                utxo_set_.AddCoin(tx.inputs[j].prevout, tx_undo[j]);
            }
        }
    }
    
    // Update chain state
    height_ = block_height - 1;
    tip_hash_ = block.header.prev_block_hash;
    UpdateSupply(block.transactions[0], false);
    
    // Remove from block index
    block_index_.erase(block.GetHash());
    
    return true;
}

uint64_t Chain::GetTotalSupply(primitives::AssetID asset) const {
    auto it = total_supply_.find(asset);
    if (it != total_supply_.end()) {
        return it->second;
    }
    return 0;
}

std::optional<BlockIndex> Chain::GetBlockIndex(const std::array<uint8_t, 32>& hash) const {
    auto it = block_index_.find(hash);
    if (it != block_index_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void Chain::Reset() {
    utxo_set_.Clear();
    height_ = 0;
    tip_hash_ = std::array<uint8_t, 32>{};
    block_index_.clear();
    total_supply_[primitives::AssetID::TALANTON] = 0;
    total_supply_[primitives::AssetID::DRACHMA] = 0;
    total_supply_[primitives::AssetID::OBOLOS] = 0;
}

} // namespace chainstate
} // namespace parthenon
