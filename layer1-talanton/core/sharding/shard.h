#ifndef PARTHENON_CORE_SHARDING_SHARD_H
#define PARTHENON_CORE_SHARDING_SHARD_H

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace parthenon {
namespace sharding {

/**
 * Shard Configuration
 * Defines the parameters for horizontal blockchain partitioning
 */
struct ShardConfig {
    uint32_t shard_id;
    uint32_t total_shards;
    uint32_t cross_shard_links;

    ShardConfig() : shard_id(0), total_shards(1), cross_shard_links(0) {}
    ShardConfig(uint32_t id, uint32_t total, uint32_t links = 0)
        : shard_id(id), total_shards(total), cross_shard_links(links) {}
};

/**
 * Shard Identifier
 * Determines which shard handles a particular transaction or account
 */
class ShardIdentifier {
  public:
    /**
     * Calculate shard ID from address
     * Uses modulo of address hash for uniform distribution
     */
    static uint32_t GetShardForAddress(const std::vector<uint8_t>& address, uint32_t total_shards);

    /**
     * Calculate shard ID from transaction hash
     */
    static uint32_t GetShardForTransaction(const std::array<uint8_t, 32>& tx_hash,
                                           uint32_t total_shards);

    /**
     * Verify if transaction belongs to shard
     */
    static bool BelongsToShard(const std::vector<uint8_t>& address, uint32_t shard_id,
                               uint32_t total_shards);
};

/**
 * Cross-Shard Transaction
 * Handles transactions that span multiple shards
 */
struct CrossShardTx {
    std::array<uint8_t, 32> tx_hash;
    uint32_t source_shard;
    uint32_t destination_shard;
    std::vector<uint8_t> proof;
    uint32_t block_height;

    CrossShardTx() : source_shard(0), destination_shard(0), block_height(0) {}
};

/**
 * Shard State Manager
 * Manages state for a single shard
 */
class ShardStateManager {
  public:
    explicit ShardStateManager(const ShardConfig& config);
    ~ShardStateManager();

    /**
     * Get shard configuration
     */
    const ShardConfig& GetConfig() const { return config_; }

    /**
     * Check if address belongs to this shard
     */
    bool OwnsAddress(const std::vector<uint8_t>& address) const;

    /**
     * Process cross-shard transaction
     */
    bool ProcessCrossShardTx(const CrossShardTx& tx);

    /**
     * Get pending cross-shard transactions
     */
    std::vector<CrossShardTx> GetPendingCrossShardTxs() const;

    /**
     * Validate cross-shard proof
     */
    bool ValidateCrossShardProof(const CrossShardTx& tx) const;

  private:
    ShardConfig config_;
    std::map<std::array<uint8_t, 32>, CrossShardTx> pending_cross_shard_;
};

/**
 * Shard Coordinator
 * Coordinates communication between shards
 */
class ShardCoordinator {
  public:
    ShardCoordinator(uint32_t total_shards);
    ~ShardCoordinator();

    /**
     * Register a shard
     */
    bool RegisterShard(uint32_t shard_id, std::shared_ptr<ShardStateManager> manager);

    /**
     * Route transaction to appropriate shard
     */
    uint32_t RouteTransaction(const std::array<uint8_t, 32>& tx_hash);

    /**
     * Handle cross-shard communication
     */
    bool RouteCrossShardTx(const CrossShardTx& tx);

    /**
     * Get shard manager
     */
    std::shared_ptr<ShardStateManager> GetShard(uint32_t shard_id);

    /**
     * Get total number of shards
     */
    uint32_t GetTotalShards() const { return total_shards_; }

  private:
    uint32_t total_shards_;
    std::map<uint32_t, std::shared_ptr<ShardStateManager>> shards_;
};

}  // namespace sharding
}  // namespace parthenon

#endif  // PARTHENON_CORE_SHARDING_SHARD_H
