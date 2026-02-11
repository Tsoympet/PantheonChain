// ParthenonChain - Mempool
// Transaction pool with fee-based prioritization

#ifndef PARTHENON_MEMPOOL_MEMPOOL_H
#define PARTHENON_MEMPOOL_MEMPOOL_H

#include "chainstate/utxo.h"
#include "primitives/transaction.h"

#include <map>
#include <optional>
#include <set>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace mempool {

/**
 * Transaction entry in mempool
 */
struct MempoolEntry {
    primitives::Transaction tx;
    uint64_t fee;             // Total fee
    uint64_t fee_rate;        // Fee per byte
    uint32_t time;            // Entry time
    uint32_t height;          // Height when added
    size_t size;              // Transaction size in bytes
    uint64_t ancestor_fee;    // Total fee including ancestors
    size_t ancestor_size;     // Total size including ancestors
    uint32_t ancestor_count;  // Number of unconfirmed ancestors
    bool signals_rbf;         // Signals replace-by-fee

    MempoolEntry()
        : fee(0),
          fee_rate(0),
          time(0),
          height(0),
          size(0),
          ancestor_fee(0),
          ancestor_size(0),
          ancestor_count(0),
          signals_rbf(false) {}

    MempoolEntry(const primitives::Transaction& t, uint64_t f, uint32_t tm, uint32_t h,
                 bool rbf = false)
        : tx(t), fee(f), time(tm), height(h), signals_rbf(rbf) {
        size = tx.Serialize().size();
        fee_rate = (size > 0) ? (fee / size) : 0;
        // Initialize ancestor values to self
        ancestor_fee = fee;
        ancestor_size = size;
        ancestor_count = 0;
    }

    /**
     * Get effective fee rate (including ancestors for CPFP)
     */
    uint64_t GetEffectiveFeeRate() const {
        size_t total_size = ancestor_size > 0 ? ancestor_size : size;
        uint64_t total_fee = ancestor_fee > 0 ? ancestor_fee : fee;
        return (total_size > 0) ? (total_fee / total_size) : 0;
    }

    /**
     * Compare by effective fee rate (for priority ordering with CPFP)
     */
    bool operator<(const MempoolEntry& other) const {
        uint64_t my_rate = GetEffectiveFeeRate();
        uint64_t other_rate = other.GetEffectiveFeeRate();

        // Higher effective fee rate = higher priority
        if (my_rate != other_rate) {
            return my_rate > other_rate;
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
    Mempool()
        : total_size_(0),
          max_size_(300 * 1024 * 1024),  // 300 MB default
          min_relay_fee_(1) {}           // 1 satoshi per KB (very permissive for testing)

    /**
     * Add transaction to mempool
     *
     * @param tx Transaction to add
     * @param utxo_set UTXO set to validate against
     * @param height Current blockchain height
     * @return true if added successfully
     */
    bool AddTransaction(const primitives::Transaction& tx, const chainstate::UTXOSet& utxo_set,
                        uint32_t height);

    /**
     * Remove transaction from mempool
     */
    bool RemoveTransaction(const std::array<uint8_t, 32>& txid);

    /**
     * Get transaction from mempool
     */
    std::optional<primitives::Transaction>
    GetTransaction(const std::array<uint8_t, 32>& txid) const;

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
    void RemoveConflicting(const std::vector<primitives::Transaction>& confirmed_txs,
                           const chainstate::UTXOSet& utxo_set, uint32_t height);

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

    /**
     * Attempt to replace transaction with higher fee (RBF)
     *
     * @param tx New transaction that replaces existing one
     * @param utxo_set UTXO set to validate against
     * @param height Current blockchain height
     * @return true if replacement was successful
     */
    bool ReplaceTransaction(const primitives::Transaction& tx, const chainstate::UTXOSet& utxo_set,
                            uint32_t height);

    /**
     * Get transactions with their descendants (for CPFP)
     *
     * @param max_count Maximum number of transaction packages to return
     * @return Vector of transaction packages (parent + children)
     */
    std::vector<std::vector<primitives::Transaction>>
    GetTransactionPackages(size_t max_count) const;

    /**
     * Update ancestor information for transaction (for CPFP)
     */
    void UpdateAncestorState(const std::array<uint8_t, 32>& txid);

    /**
     * Get descendant transactions
     */
    std::vector<std::array<uint8_t, 32>> GetDescendants(const std::array<uint8_t, 32>& txid) const;

  private:
    // Map from txid to mempool entry
    std::map<std::array<uint8_t, 32>, MempoolEntry> transactions_;

    // Set ordered by fee rate (for priority)
    std::set<MempoolEntry> priority_queue_;

    // Track spent outpoints (for conflict detection)
    std::map<primitives::OutPoint, std::array<uint8_t, 32>> spent_outpoints_;

    // Track parent-child relationships (for CPFP)
    std::map<std::array<uint8_t, 32>, std::vector<std::array<uint8_t, 32>>> children_;
    std::map<std::array<uint8_t, 32>, std::vector<std::array<uint8_t, 32>>> parents_;

    size_t total_size_;       // Total size in bytes
    size_t max_size_;         // Maximum size limit
    uint64_t min_relay_fee_;  // Minimum fee rate for relay (satoshis per KB)

    // RBF constants
    static constexpr uint64_t MIN_RBF_FEE_INCREMENT =
        1000;  // Minimum fee increase for RBF (satoshis)
    static constexpr double MIN_RBF_FEE_RATE_MULTIPLIER = 1.1;  // Minimum 10% fee rate increase

    /**
     * Calculate fee for a transaction
     */
    uint64_t CalculateFee(const primitives::Transaction& tx,
                          const chainstate::UTXOSet& utxo_set) const;

    /**
     * Validate transaction for mempool acceptance
     */
    bool ValidateTransaction(const primitives::Transaction& tx, const chainstate::UTXOSet& utxo_set,
                             uint32_t height) const;

    /**
     * Check for conflicts with existing mempool transactions
     */
    bool HasConflict(const primitives::Transaction& tx) const;

    /**
     * Evict low-fee transactions if mempool is full
     */
    void EvictTransactions(size_t required_space);

    /**
     * Check if transaction signals RBF
     */
    bool CheckRBFSignaling(const primitives::Transaction& tx) const;

    /**
     * Get all transactions that would be replaced by this one
     */
    std::vector<std::array<uint8_t, 32>>
    GetConflictingTransactions(const primitives::Transaction& tx) const;

    /**
     * Calculate total fees that would be replaced
     */
    uint64_t
    CalculateReplacedFees(const std::vector<std::array<uint8_t, 32>>& replaced_txids) const;

    /**
     * Update parent-child relationships
     */
    void UpdateRelationships(const std::array<uint8_t, 32>& txid);

    /**
     * Remove from relationship tracking
     */
    void RemoveFromRelationships(const std::array<uint8_t, 32>& txid);
};

}  // namespace mempool
}  // namespace parthenon

#endif  // PARTHENON_MEMPOOL_MEMPOOL_H
