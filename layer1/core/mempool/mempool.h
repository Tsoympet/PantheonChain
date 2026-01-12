// ParthenonChain - Mempool
// Transaction pool with fee-based prioritization

#ifndef PARTHENON_MEMPOOL_MEMPOOL_H
#define PARTHENON_MEMPOOL_MEMPOOL_H

#include "primitives/transaction.h"
#include "chainstate/utxo.h"
#include <map>
#include <set>
#include <vector>
#include <optional>

namespace parthenon {
namespace mempool {

/**
 * Transaction entry in mempool
 */
struct MempoolEntry {
    primitives::Transaction tx;
    uint64_t fee;              // Total fee
    uint64_t fee_rate;         // Fee per byte
    uint32_t time;             // Entry time
    uint32_t height;           // Height when added
    size_t size;               // Transaction size in bytes
    
    MempoolEntry() : fee(0), fee_rate(0), time(0), height(0), size(0) {}
    
    MempoolEntry(const primitives::Transaction& t, uint64_t f, uint32_t tm, uint32_t h)
        : tx(t), fee(f), time(tm), height(h) {
        size = tx.Serialize().size();
        fee_rate = (size > 0) ? (fee / size) : 0;
    }
    
    /**
     * Compare by fee rate (for priority ordering)
     */
    bool operator<(const MempoolEntry& other) const {
        // Higher fee rate = higher priority
        if (fee_rate != other.fee_rate) {
            return fee_rate > other.fee_rate;
        }
        // If same fee rate, older transaction has priority
        return time < other.time;
    }
};

/**
 * Mempool manages the set of unconfirmed transactions
 */
class Mempool {
public:
    Mempool() : total_size_(0), max_size_(300 * 1024 * 1024), // 300 MB default
                min_relay_fee_(1) {}  // 1 satoshi per KB (very permissive for testing)
    
    /**
     * Add transaction to mempool
     * 
     * @param tx Transaction to add
     * @param utxo_set UTXO set to validate against
     * @param height Current blockchain height
     * @return true if added successfully
     */
    bool AddTransaction(
        const primitives::Transaction& tx,
        const chainstate::UTXOSet& utxo_set,
        uint32_t height
    );
    
    /**
     * Remove transaction from mempool
     */
    bool RemoveTransaction(const std::array<uint8_t, 32>& txid);
    
    /**
     * Get transaction from mempool
     */
    std::optional<primitives::Transaction> GetTransaction(
        const std::array<uint8_t, 32>& txid
    ) const;
    
    /**
     * Check if transaction exists in mempool
     */
    bool HasTransaction(const std::array<uint8_t, 32>& txid) const;
    
    /**
     * Get all transactions ordered by fee rate (highest first)
     */
    std::vector<primitives::Transaction> GetTransactionsByFeeRate(size_t max_count) const;
    
    /**
     * Remove transactions that are now invalid (e.g., after a block connection)
     */
    void RemoveConflicting(
        const std::vector<primitives::Transaction>& confirmed_txs,
        const chainstate::UTXOSet& utxo_set,
        uint32_t height
    );
    
    /**
     * Get mempool size in bytes
     */
    size_t GetSize() const { return total_size_; }
    
    /**
     * Get number of transactions
     */
    size_t GetTransactionCount() const { return transactions_.size(); }
    
    /**
     * Clear all transactions
     */
    void Clear();
    
    /**
     * Set maximum mempool size
     */
    void SetMaxSize(size_t size) { max_size_ = size; }
    
    /**
     * Set minimum relay fee rate
     */
    void SetMinRelayFeeRate(uint64_t fee_rate) { min_relay_fee_ = fee_rate; }
    
    /**
     * Get estimated fee rate for inclusion in next N blocks
     */
    uint64_t EstimateFeeRate(uint32_t num_blocks) const;
    
private:
    // Map from txid to mempool entry
    std::map<std::array<uint8_t, 32>, MempoolEntry> transactions_;
    
    // Set ordered by fee rate (for priority)
    std::set<MempoolEntry> priority_queue_;
    
    // Track spent outpoints (for conflict detection)
    std::map<primitives::OutPoint, std::array<uint8_t, 32>> spent_outpoints_;
    
    size_t total_size_;     // Total size in bytes
    size_t max_size_;       // Maximum size limit
    uint64_t min_relay_fee_; // Minimum fee rate for relay (satoshis per KB)
    
    /**
     * Calculate fee for a transaction
     */
    uint64_t CalculateFee(
        const primitives::Transaction& tx,
        const chainstate::UTXOSet& utxo_set
    ) const;
    
    /**
     * Validate transaction for mempool acceptance
     */
    bool ValidateTransaction(
        const primitives::Transaction& tx,
        const chainstate::UTXOSet& utxo_set,
        uint32_t height
    ) const;
    
    /**
     * Check for conflicts with existing mempool transactions
     */
    bool HasConflict(const primitives::Transaction& tx) const;
    
    /**
     * Evict low-fee transactions if mempool is full
     */
    void EvictTransactions(size_t required_space);
};

} // namespace mempool
} // namespace parthenon

#endif // PARTHENON_MEMPOOL_MEMPOOL_H
