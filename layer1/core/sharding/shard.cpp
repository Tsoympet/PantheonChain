#include "shard.h"
#include "layer1/core/crypto/sha256.h"
#include <cstring>
#include <algorithm>

namespace parthenon {
namespace sharding {

// ShardIdentifier Implementation
uint32_t ShardIdentifier::GetShardForAddress(
    const std::vector<uint8_t>& address,
    uint32_t total_shards)
{
    if (total_shards == 0 || total_shards == 1) {
        return 0;
    }
    
    // Hash the address for uniform distribution
    std::array<uint8_t, 32> hash;
    crypto::SHA256 hasher;
    hasher.Write(address.data(), address.size());
    hasher.Finalize(hash.data());
    
    // Use first 4 bytes as uint32
    uint32_t value;
    std::memcpy(&value, hash.data(), sizeof(uint32_t));
    
    return value % total_shards;
}

uint32_t ShardIdentifier::GetShardForTransaction(
    const std::array<uint8_t, 32>& tx_hash,
    uint32_t total_shards)
{
    if (total_shards == 0 || total_shards == 1) {
        return 0;
    }
    
    // Use first 4 bytes of transaction hash
    uint32_t value;
    std::memcpy(&value, tx_hash.data(), sizeof(uint32_t));
    
    return value % total_shards;
}

bool ShardIdentifier::BelongsToShard(
    const std::vector<uint8_t>& address,
    uint32_t shard_id,
    uint32_t total_shards)
{
    return GetShardForAddress(address, total_shards) == shard_id;
}

// ShardStateManager Implementation
ShardStateManager::ShardStateManager(const ShardConfig& config)
    : config_(config)
{
}

ShardStateManager::~ShardStateManager() = default;

bool ShardStateManager::OwnsAddress(const std::vector<uint8_t>& address) const
{
    return ShardIdentifier::BelongsToShard(
        address, 
        config_.shard_id, 
        config_.total_shards
    );
}

bool ShardStateManager::ProcessCrossShardTx(const CrossShardTx& tx)
{
    // Verify the transaction belongs to this shard
    if (tx.destination_shard != config_.shard_id) {
        return false;
    }
    
    // Validate the cross-shard proof
    if (!ValidateCrossShardProof(tx)) {
        return false;
    }
    
    // Store in pending until confirmed
    pending_cross_shard_[tx.tx_hash] = tx;
    
    return true;
}

std::vector<CrossShardTx> ShardStateManager::GetPendingCrossShardTxs() const
{
    std::vector<CrossShardTx> result;
    result.reserve(pending_cross_shard_.size());
    
    for (const auto& [hash, tx] : pending_cross_shard_) {
        result.push_back(tx);
    }
    
    return result;
}

bool ShardStateManager::ValidateCrossShardProof(const CrossShardTx& tx) const
{
    // Verify proof is not empty
    if (tx.proof.empty()) {
        return false;
    }
    
    // Verify source and destination are different
    if (tx.source_shard == tx.destination_shard) {
        return false;
    }
    
    // Verify shards are within valid range
    if (tx.source_shard >= config_.total_shards ||
        tx.destination_shard >= config_.total_shards) {
        return false;
    }
    
    // Additional proof validation would go here
    // For now, basic validation passes
    return true;
}

// ShardCoordinator Implementation
ShardCoordinator::ShardCoordinator(uint32_t total_shards)
    : total_shards_(total_shards)
{
}

ShardCoordinator::~ShardCoordinator() = default;

bool ShardCoordinator::RegisterShard(
    uint32_t shard_id,
    std::shared_ptr<ShardStateManager> manager)
{
    if (shard_id >= total_shards_) {
        return false;
    }
    
    shards_[shard_id] = manager;
    return true;
}

uint32_t ShardCoordinator::RouteTransaction(const std::array<uint8_t, 32>& tx_hash)
{
    return ShardIdentifier::GetShardForTransaction(tx_hash, total_shards_);
}

bool ShardCoordinator::RouteCrossShardTx(const CrossShardTx& tx)
{
    // Get destination shard
    auto it = shards_.find(tx.destination_shard);
    if (it == shards_.end()) {
        return false;
    }
    
    // Forward to destination shard
    return it->second->ProcessCrossShardTx(tx);
}

std::shared_ptr<ShardStateManager> ShardCoordinator::GetShard(uint32_t shard_id)
{
    auto it = shards_.find(shard_id);
    if (it == shards_.end()) {
        return nullptr;
    }
    return it->second;
}

} // namespace sharding
} // namespace parthenon
