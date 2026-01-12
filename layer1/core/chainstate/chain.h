// ParthenonChain - Extended Chain State with UTXO Management
// Consensus-critical: Block connection/disconnection and UTXO tracking

#ifndef PARTHENON_CHAINSTATE_CHAIN_H
#define PARTHENON_CHAINSTATE_CHAIN_H

#include "utxo.h"
#include "primitives/block.h"
#include <array>
#include <vector>
#include <map>

namespace parthenon {
namespace chainstate {

/**
 * BlockIndex stores metadata about a block in the chain
 */
struct BlockIndex {
    std::array<uint8_t, 32> hash;
    std::array<uint8_t, 32> prev_hash;
    uint32_t height;
    uint32_t timestamp;
    uint32_t bits;
    
    // Chain work (cumulative difficulty)
    // For simplicity, we track number of blocks
    uint64_t chain_work;
    
    BlockIndex() : hash{}, prev_hash{}, height(0), timestamp(0), bits(0), chain_work(0) {}
    
    BlockIndex(const primitives::BlockHeader& header, uint32_t h, uint64_t work)
        : hash(header.GetHash()), prev_hash(header.prev_block_hash),
          height(h), timestamp(header.timestamp), bits(header.bits),
          chain_work(work) {}
};

/**
 * Chain manages the blockchain state including UTXO set and block indices
 */
class Chain {
public:
    Chain();
    
    /**
     * Connect a block to the active chain
     * - Validates all transactions
     * - Updates UTXO set
     * - Records undo data
     * 
     * @param block Block to connect
     * @param undo Output parameter for undo data
     * @return true if block was successfully connected
     */
    bool ConnectBlock(const primitives::Block& block, BlockUndo& undo);
    
    /**
     * Disconnect a block from the active chain
     * - Restores UTXO set using undo data
     * - Reverts state changes
     * 
     * @param block Block to disconnect
     * @param undo Undo data for this block
     * @return true if block was successfully disconnected
     */
    bool DisconnectBlock(const primitives::Block& block, const BlockUndo& undo);
    
    /**
     * Get current chain height
     */
    uint32_t GetHeight() const { return height_; }
    
    /**
     * Get the UTXO set
     */
    const UTXOSet& GetUTXOSet() const { return utxo_set_; }
    UTXOSet& GetUTXOSet() { return utxo_set_; }
    
    /**
     * Get total supply for an asset
     */
    uint64_t GetTotalSupply(primitives::AssetID asset) const;
    
    /**
     * Get block index by hash
     */
    std::optional<BlockIndex> GetBlockIndex(const std::array<uint8_t, 32>& hash) const;
    
    /**
     * Get tip (best block) hash
     */
    const std::array<uint8_t, 32>& GetTip() const { return tip_hash_; }
    
    /**
     * Reset chain to genesis state
     */
    void Reset();
    
private:
    UTXOSet utxo_set_;
    uint32_t height_;
    std::array<uint8_t, 32> tip_hash_;
    
    // Block index (hash -> BlockIndex)
    std::map<std::array<uint8_t, 32>, BlockIndex> block_index_;
    
    // Total supply tracking
    std::map<primitives::AssetID, uint64_t> total_supply_;
    
    /**
     * Validate transaction against UTXO set
     */
    bool ValidateTransaction(const primitives::Transaction& tx, uint32_t height) const;
    
    /**
     * Update supply tracking when connecting a block
     */
    void UpdateSupply(const primitives::Transaction& coinbase, bool connect);
};

} // namespace chainstate
} // namespace parthenon

#endif // PARTHENON_CHAINSTATE_CHAIN_H
