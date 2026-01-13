// ParthenonChain - Transaction Indexer
// Indexes all blockchain transactions for fast queries

#pragma once

#include "primitives/transaction.h"
#include <string>
#include <vector>
#include <optional>

namespace parthenon {
namespace layer2 {
namespace indexers {

/**
 * Transaction Indexer
 * 
 * Indexes all transactions in the blockchain for fast lookups
 * by address, asset, time range, etc.
 */
class TxIndexer {
public:
    TxIndexer();
    ~TxIndexer();
    
    /**
     * Index a transaction
     */
    void IndexTransaction(const primitives::Transaction& tx, uint32_t height, uint32_t block_time);
    
    /**
     * Get transactions for an address
     */
    std::vector<primitives::Transaction> GetTransactionsByAddress(
        const std::vector<uint8_t>& address,
        uint32_t limit = 100
    );
    
    /**
     * Get transaction by ID
     */
    std::optional<primitives::Transaction> GetTransactionById(
        const std::array<uint8_t, 32>& txid
    );
    
private:
    // TODO: Implement database backend (LevelDB, RocksDB, etc.)
};

} // namespace indexers
} // namespace layer2
} // namespace parthenon
