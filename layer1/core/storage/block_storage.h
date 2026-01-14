// ParthenonChain - Block Storage Module
// LevelDB-based persistent block storage

#pragma once

#include "primitives/block.h"

#include <leveldb/db.h>
#include <memory>
#include <optional>
#include <string>

namespace parthenon {
namespace storage {

/**
 * BlockStorage provides persistent storage for blockchain blocks using LevelDB
 *
 * Storage layout:
 * - "b{height}" -> serialized Block
 * - "h{hash}" -> height (uint32_t)
 * - "meta:height" -> current chain height
 * - "meta:best_hash" -> hash of best block
 */
class BlockStorage {
  public:
    /**
     * Open block storage database
     * @param db_path Path to LevelDB database directory
     * @return true if opened successfully
     */
    bool Open(const std::string& db_path);

    /**
     * Close the database
     */
    void Close();

    /**
     * Store a block at given height
     * @param block Block to store
     * @param height Block height
     * @return true if stored successfully
     */
    bool StoreBlock(const primitives::Block& block, uint32_t height);

    /**
     * Retrieve a block by height
     * @param height Block height
     * @return Block if found, nullopt otherwise
     */
    std::optional<primitives::Block> GetBlockByHeight(uint32_t height);

    /**
     * Retrieve a block by hash
     * @param hash Block hash
     * @return Block if found, nullopt otherwise
     */
    std::optional<primitives::Block> GetBlockByHash(const std::array<uint8_t, 32>& hash);

    /**
     * Get current chain height
     * @return Chain height (0 if no blocks)
     */
    uint32_t GetHeight();

    /**
     * Update chain tip metadata
     * @param height New chain height
     * @param best_hash Hash of best block
     */
    bool UpdateChainTip(uint32_t height, const std::array<uint8_t, 32>& best_hash);

    /**
     * Check if database is open
     */
    bool IsOpen() const { return db_ != nullptr; }

  private:
    std::unique_ptr<leveldb::DB> db_;

    // Helper functions
    std::string HeightKey(uint32_t height);
    std::string HashKey(const std::array<uint8_t, 32>& hash);
    std::string SerializeBlock(const primitives::Block& block);
    std::optional<primitives::Block> DeserializeBlock(const std::string& data);
};

}  // namespace storage
}  // namespace parthenon
