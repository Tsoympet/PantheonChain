// ParthenonChain - UTXO (Unspent Transaction Output) Set
// Consensus-critical: UTXO tracking and management

#ifndef PARTHENON_CHAINSTATE_UTXO_H
#define PARTHENON_CHAINSTATE_UTXO_H

#include "primitives/transaction.h"
#include "primitives/asset.h"
#include <map>
#include <optional>

namespace parthenon {
namespace chainstate {

/**
 * Coin represents a single unspent transaction output
 * Contains the output data and metadata about when it was created
 */
class Coin {
public:
    primitives::TxOutput output;
    uint32_t height;        // Height at which this output was created
    bool is_coinbase;       // Whether this is from a coinbase transaction
    
    Coin() : height(0), is_coinbase(false) {}
    
    Coin(const primitives::TxOutput& out, uint32_t h, bool coinbase)
        : output(out), height(h), is_coinbase(coinbase) {}
    
    /**
     * Check if this coin is spendable at given height
     * Coinbase outputs require maturity (100 blocks)
     */
    bool IsSpendable(uint32_t current_height) const {
        if (!is_coinbase) {
            return true;
        }
        // Coinbase maturity: must wait 100 blocks
        return current_height >= height + 100;
    }
};

/**
 * UTXOSet maintains the set of all unspent transaction outputs
 * This is the core data structure for validation
 */
class UTXOSet {
public:
    UTXOSet() = default;
    
    /**
     * Add a new unspent output
     */
    void AddCoin(const primitives::OutPoint& outpoint, const Coin& coin);
    
    /**
     * Spend an output (remove from UTXO set)
     */
    bool SpendCoin(const primitives::OutPoint& outpoint);
    
    /**
     * Get a coin if it exists in the UTXO set
     */
    std::optional<Coin> GetCoin(const primitives::OutPoint& outpoint) const;
    
    /**
     * Check if an output exists in the UTXO set
     */
    bool HaveCoin(const primitives::OutPoint& outpoint) const;
    
    /**
     * Get total number of UTXOs
     */
    size_t GetSize() const { return utxos_.size(); }
    
    /**
     * Clear all UTXOs (for reset)
     */
    void Clear() { utxos_.clear(); }
    
    /**
     * Get all UTXOs (for testing/debugging)
     */
    const std::map<primitives::OutPoint, Coin>& GetUTXOs() const { return utxos_; }
    
private:
    // Map from OutPoint to Coin
    std::map<primitives::OutPoint, Coin> utxos_;
};

/**
 * BlockUndo stores information needed to disconnect a block
 * Contains all the coins that were spent when the block was connected
 */
class BlockUndo {
public:
    // Coins spent by each transaction in the block (except coinbase)
    // Indexed by transaction index (starting from 1, since coinbase at 0 spends nothing)
    std::vector<std::vector<Coin>> tx_undo;
    
    BlockUndo() = default;
    
    /**
     * Add undo data for a transaction
     */
    void AddTxUndo(const std::vector<Coin>& coins) {
        tx_undo.push_back(coins);
    }
    
    /**
     * Check if this undo data is empty
     */
    bool IsEmpty() const { return tx_undo.empty(); }
};

} // namespace chainstate
} // namespace parthenon

#endif // PARTHENON_CHAINSTATE_UTXO_H
