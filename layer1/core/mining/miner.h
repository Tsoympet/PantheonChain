// ParthenonChain - Mining Module
// Proof-of-Work block mining and template construction

#pragma once

#include "primitives/block.h"
#include "primitives/transaction.h"
#include "consensus/difficulty.h"
#include "consensus/issuance.h"
#include "chainstate/chainstate.h"
#include "evm/state.h"
#include <vector>
#include <memory>
#include <optional>

namespace parthenon {
namespace mining {

using uint256_t = evm::uint256_t;

/**
 * BlockTemplate contains all data needed to mine a new block
 */
struct BlockTemplate {
    primitives::Block block;              // Block being mined
    uint64_t total_fees;                  // Total transaction fees
    std::vector<primitives::AssetAmount> coinbase_rewards;  // Rewards per asset
    uint32_t height;                      // Block height
    uint256_t target;                     // PoW target
};

/**
 * MiningStatus tracks mining progress
 */
struct MiningStatus {
    bool is_mining;
    uint32_t height;
    uint64_t hashrate;      // Hashes per second
    uint64_t total_hashes;
    std::array<uint8_t, 32> current_block_hash;
};

/**
 * Miner handles block template construction and PoW mining
 */
class Miner {
public:
    /**
     * Construct a miner
     * @param chainstate Reference to chain state
     * @param coinbase_address Address to receive mining rewards
     */
    Miner(chainstate::ChainState& chainstate, const std::vector<uint8_t>& coinbase_pubkey);
    
    /**
     * Create a new block template from mempool transactions
     * @param max_transactions Maximum number of transactions to include
     * @return Block template ready for mining
     */
    std::optional<BlockTemplate> CreateBlockTemplate(size_t max_transactions = 1000);
    
    /**
     * Mine a block (find valid nonce)
     * @param block_template Template to mine
     * @param max_iterations Maximum mining iterations (0 = unlimited)
     * @return Mined block if successful
     */
    std::optional<primitives::Block> MineBlock(
        const BlockTemplate& block_template,
        uint64_t max_iterations = 0
    );
    
    /**
     * Verify that a block meets PoW difficulty requirement
     * @param block Block to verify
     * @param target Target difficulty
     * @return true if block hash < target
     */
    static bool VerifyProofOfWork(const primitives::Block& block, const uint256_t& target);
    
    /**
     * Get current mining status
     */
    MiningStatus GetStatus() const;
    
    /**
     * Stop mining
     */
    void StopMining();
    
private:
    /**
     * Create coinbase transaction
     * @param height Block height
     * @param rewards Coinbase rewards per asset
     * @return Coinbase transaction
     */
    primitives::Transaction CreateCoinbaseTransaction(
        uint32_t height,
        const std::vector<primitives::AssetAmount>& rewards
    );
    
    /**
     * Calculate total fees from transactions
     * @param transactions Transactions in block
     * @return Total fees per asset
     */
    std::map<primitives::AssetID, uint64_t> CalculateFees(
        const std::vector<primitives::Transaction>& transactions
    );
    
    /**
     * Select transactions from mempool
     * @param max_count Maximum transactions to select
     * @return Selected transactions (prioritized by fee)
     */
    std::vector<primitives::Transaction> SelectTransactions(size_t max_count);
    
    /**
     * Compute merkle root of transactions
     * @param transactions Block transactions
     * @return Merkle root hash
     */
    std::array<uint8_t, 32> ComputeMerkleRoot(
        const std::vector<primitives::Transaction>& transactions
    );
    
    chainstate::ChainState& chainstate_;
    std::vector<uint8_t> coinbase_pubkey_;
    
    // Mining state
    bool is_mining_;
    uint64_t hashrate_;
    uint64_t total_hashes_;
};

} // namespace mining
} // namespace parthenon
