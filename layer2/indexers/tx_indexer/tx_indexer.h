// ParthenonChain - Transaction Indexer
// Indexes all blockchain transactions for fast queries

#pragma once

#include "primitives/transaction.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

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
     * Open the indexer database
     */
    bool Open(const std::string& db_path);

    /**
     * Close the indexer database
     */
    void Close();

    /**
     * Index a transaction
     */
    void IndexTransaction(const primitives::Transaction& tx, uint32_t height, uint32_t block_time);

    /**
     * Get transactions for an address
     */
    std::vector<primitives::Transaction>
    GetTransactionsByAddress(const std::vector<uint8_t>& address, uint32_t limit = 100);

    /**
     * Get transaction by ID
     */
    std::optional<primitives::Transaction> GetTransactionById(const std::array<uint8_t, 32>& txid);

    /**
     * Get total number of indexed transactions
     */
    size_t GetTransactionCount() const;

    /**
     * Get recent transactions
     */
    std::vector<primitives::Transaction> GetRecentTransactions(uint32_t limit = 100);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace indexers
}  // namespace layer2
}  // namespace parthenon
