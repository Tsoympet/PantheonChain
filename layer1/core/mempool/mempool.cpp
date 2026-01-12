// ParthenonChain - Mempool Implementation

#include "mempool.h"
#include "validation/validation.h"
#include <algorithm>
#include <ctime>

namespace parthenon {
namespace mempool {

uint64_t Mempool::CalculateFee(
    const primitives::Transaction& tx,
    const chainstate::UTXOSet& utxo_set
) const {
    if (tx.IsCoinbase()) {
        return 0;
    }
    
    uint64_t input_value = 0;
    uint64_t output_value = 0;
    
    // Calculate input value
    for (const auto& input : tx.inputs) {
        auto coin = utxo_set.GetCoin(input.prevout);
        if (coin) {
            input_value += coin->output.value.amount;
        }
    }
    
    // Calculate output value
    for (const auto& output : tx.outputs) {
        output_value += output.value.amount;
    }
    
    // Fee is the difference (per asset, but we simplify here)
    return (input_value > output_value) ? (input_value - output_value) : 0;
}

bool Mempool::ValidateTransaction(
    const primitives::Transaction& tx,
    const chainstate::UTXOSet& utxo_set,
    uint32_t height
) const {
    // Use validation module
    auto error = validation::TransactionValidator::ValidateStructure(tx);
    if (error) return false;
    
    error = validation::TransactionValidator::ValidateAgainstUTXO(tx, utxo_set, height);
    if (error) return false;
    
    // Check fee rate meets minimum (with overflow protection)
    size_t tx_size = tx.Serialize().size();
    uint64_t fee = CalculateFee(tx, utxo_set);
    
    // Calculate fee rate safely (satoshis per KB)
    uint64_t fee_rate = 0;
    if (tx_size > 0) {
        // Check for overflow: fee * 1000 < UINT64_MAX
        if (fee <= UINT64_MAX / 1000) {
            fee_rate = (fee * 1000) / tx_size;
        } else {
            // Very large fee, compute differently
            fee_rate = fee / (tx_size / 1000);
        }
    }
    
    if (fee_rate < min_relay_fee_) {
        return false;
    }
    
    return true;
}

bool Mempool::HasConflict(const primitives::Transaction& tx) const {
    // Check if any input is already spent by a mempool transaction
    for (const auto& input : tx.inputs) {
        if (spent_outpoints_.find(input.prevout) != spent_outpoints_.end()) {
            return true; // Conflict found
        }
    }
    return false;
}

bool Mempool::AddTransaction(
    const primitives::Transaction& tx,
    const chainstate::UTXOSet& utxo_set,
    uint32_t height
) {
    auto txid = tx.GetTxID();
    
    // Check if already in mempool
    if (transactions_.find(txid) != transactions_.end()) {
        return false;
    }
    
    // Validate transaction
    if (!ValidateTransaction(tx, utxo_set, height)) {
        return false;
    }
    
    // Check for conflicts
    if (HasConflict(tx)) {
        return false; // Double-spend attempt
    }
    
    // Calculate fee and size
    uint64_t fee = CalculateFee(tx, utxo_set);
    uint32_t time = static_cast<uint32_t>(std::time(nullptr));
    
    MempoolEntry entry(tx, fee, time, height);
    
    // Check if we need to evict transactions
    if (total_size_ + entry.size > max_size_) {
        EvictTransactions(entry.size);
        
        // If still no space, reject
        if (total_size_ + entry.size > max_size_) {
            return false;
        }
    }
    
    // Add to mempool
    transactions_[txid] = entry;
    priority_queue_.insert(entry);
    total_size_ += entry.size;
    
    // Track spent outpoints
    for (const auto& input : tx.inputs) {
        spent_outpoints_[input.prevout] = txid;
    }
    
    return true;
}

bool Mempool::RemoveTransaction(const std::array<uint8_t, 32>& txid) {
    auto it = transactions_.find(txid);
    if (it == transactions_.end()) {
        return false;
    }
    
    const auto& entry = it->second;
    
    // Remove from priority queue
    priority_queue_.erase(entry);
    
    // Remove spent outpoints
    for (const auto& input : entry.tx.inputs) {
        spent_outpoints_.erase(input.prevout);
    }
    
    // Update size
    total_size_ -= entry.size;
    
    // Remove from map
    transactions_.erase(it);
    
    return true;
}

std::optional<primitives::Transaction> Mempool::GetTransaction(
    const std::array<uint8_t, 32>& txid
) const {
    auto it = transactions_.find(txid);
    if (it != transactions_.end()) {
        return it->second.tx;
    }
    return std::nullopt;
}

bool Mempool::HasTransaction(const std::array<uint8_t, 32>& txid) const {
    return transactions_.find(txid) != transactions_.end();
}

std::vector<primitives::Transaction> Mempool::GetTransactionsByFeeRate(size_t max_count) const {
    std::vector<primitives::Transaction> result;
    result.reserve(std::min(max_count, priority_queue_.size()));
    
    for (const auto& entry : priority_queue_) {
        if (result.size() >= max_count) break;
        result.push_back(entry.tx);
    }
    
    return result;
}

void Mempool::RemoveConflicting(
    const std::vector<primitives::Transaction>& confirmed_txs,
    const chainstate::UTXOSet& utxo_set,
    uint32_t height
) {
    // Remove confirmed transactions from mempool
    for (const auto& tx : confirmed_txs) {
        auto txid = tx.GetTxID();
        RemoveTransaction(txid);
    }
    
    // Remove transactions that are now invalid
    std::vector<std::array<uint8_t, 32>> to_remove;
    
    for (const auto& [txid, entry] : transactions_) {
        if (!ValidateTransaction(entry.tx, utxo_set, height)) {
            to_remove.push_back(txid);
        }
    }
    
    for (const auto& txid : to_remove) {
        RemoveTransaction(txid);
    }
}

void Mempool::Clear() {
    transactions_.clear();
    priority_queue_.clear();
    spent_outpoints_.clear();
    total_size_ = 0;
}

void Mempool::EvictTransactions(size_t required_space) {
    // Evict lowest fee-rate transactions until we have enough space
    while (total_size_ + required_space > max_size_ && !priority_queue_.empty()) {
        // Get lowest priority transaction (at the end of the set)
        auto it = priority_queue_.rbegin();
        auto txid = it->tx.GetTxID();
        
        RemoveTransaction(txid);
    }
}

uint64_t Mempool::EstimateFeeRate(uint32_t num_blocks) const {
    // Simple fee estimation based on current mempool
    // More sophisticated implementation would track historical data
    
    if (priority_queue_.empty()) {
        return min_relay_fee_;
    }
    
    // Estimate capacity per block (assume 1MB blocks)
    size_t block_capacity = 1024 * 1024;
    size_t total_capacity = block_capacity * num_blocks;
    
    // Find the fee rate at the cutoff point
    size_t accumulated_size = 0;
    uint64_t cutoff_fee_rate = min_relay_fee_;
    
    for (const auto& entry : priority_queue_) {
        accumulated_size += entry.size;
        if (accumulated_size >= total_capacity) {
            cutoff_fee_rate = entry.fee_rate;
            break;
        }
    }
    
    return cutoff_fee_rate;
}

} // namespace mempool
} // namespace parthenon
