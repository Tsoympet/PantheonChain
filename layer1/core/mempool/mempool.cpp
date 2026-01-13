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
    
    // Check for conflicts - if there are conflicts, try RBF
    if (HasConflict(tx)) {
        bool signals_rbf = CheckRBFSignaling(tx);
        if (signals_rbf) {
            // Attempt RBF replacement
            return ReplaceTransaction(tx, utxo_set, height);
        }
        return false; // Double-spend attempt without RBF
    }
    
    // Calculate fee and size
    uint64_t fee = CalculateFee(tx, utxo_set);
    uint32_t time = static_cast<uint32_t>(std::time(nullptr));
    bool signals_rbf = CheckRBFSignaling(tx);
    
    MempoolEntry entry(tx, fee, time, height, signals_rbf);
    
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
    
    // Update parent-child relationships for CPFP
    UpdateRelationships(txid);
    
    // Update ancestor state
    UpdateAncestorState(txid);
    
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
    
    // Remove from relationship tracking
    RemoveFromRelationships(txid);
    
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
    children_.clear();
    parents_.clear();
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

// RBF: Replace-By-Fee implementation
bool Mempool::ReplaceTransaction(
    const primitives::Transaction& tx,
    const chainstate::UTXOSet& utxo_set,
    uint32_t height
) {
    auto txid = tx.GetTxID();
    
    // Get all conflicting transactions
    auto conflicting = GetConflictingTransactions(tx);
    if (conflicting.empty()) {
        return false; // No conflicts, should use normal add
    }
    
    // Check that all conflicting transactions signal RBF
    for (const auto& conf_txid : conflicting) {
        auto it = transactions_.find(conf_txid);
        if (it == transactions_.end() || !it->second.signals_rbf) {
            return false; // Cannot replace non-RBF transaction
        }
    }
    
    // Calculate fees
    uint64_t new_fee = CalculateFee(tx, utxo_set);
    uint64_t replaced_fees = CalculateReplacedFees(conflicting);
    
    // Verify RBF rules:
    // 1. New transaction must pay higher absolute fee
    if (new_fee <= replaced_fees) {
        return false;
    }
    
    // 2. New transaction must pay at least MIN_RBF_FEE_INCREMENT more
    if (new_fee < replaced_fees + MIN_RBF_FEE_INCREMENT) {
        return false;
    }
    
    // 3. New transaction fee rate should be higher
    size_t new_size = tx.Serialize().size();
    uint64_t new_fee_rate = (new_size > 0) ? (new_fee / new_size) : 0;
    
    // Get minimum fee rate of replaced transactions
    uint64_t min_replaced_fee_rate = UINT64_MAX;
    for (const auto& conf_txid : conflicting) {
        auto it = transactions_.find(conf_txid);
        if (it != transactions_.end()) {
            min_replaced_fee_rate = std::min(min_replaced_fee_rate, it->second.fee_rate);
        }
    }
    
    if (new_fee_rate < min_replaced_fee_rate * MIN_RBF_FEE_RATE_MULTIPLIER) {
        return false;
    }
    
    // All checks passed - remove conflicting transactions
    for (const auto& conf_txid : conflicting) {
        RemoveTransaction(conf_txid);
    }
    
    // Add new transaction
    uint32_t time = static_cast<uint32_t>(std::time(nullptr));
    bool signals_rbf = CheckRBFSignaling(tx);
    MempoolEntry entry(tx, new_fee, time, height, signals_rbf);
    
    // Add to mempool
    transactions_[txid] = entry;
    priority_queue_.insert(entry);
    total_size_ += entry.size;
    
    // Track spent outpoints
    for (const auto& input : tx.inputs) {
        spent_outpoints_[input.prevout] = txid;
    }
    
    // Update relationships
    UpdateRelationships(txid);
    UpdateAncestorState(txid);
    
    return true;
}

// Check if transaction signals RBF (BIP-125)
bool Mempool::CheckRBFSignaling(const primitives::Transaction& tx) const {
    // Transaction signals RBF if any input has sequence < 0xfffffffe
    for (const auto& input : tx.inputs) {
        if (input.sequence < 0xfffffffe) {
            return true;
        }
    }
    return false;
}

std::vector<std::array<uint8_t, 32>> Mempool::GetConflictingTransactions(
    const primitives::Transaction& tx
) const {
    std::vector<std::array<uint8_t, 32>> conflicts;
    
    // Check each input for conflicts
    for (const auto& input : tx.inputs) {
        auto it = spent_outpoints_.find(input.prevout);
        if (it != spent_outpoints_.end()) {
            conflicts.push_back(it->second);
        }
    }
    
    // Remove duplicates
    std::sort(conflicts.begin(), conflicts.end());
    conflicts.erase(std::unique(conflicts.begin(), conflicts.end()), conflicts.end());
    
    return conflicts;
}

uint64_t Mempool::CalculateReplacedFees(
    const std::vector<std::array<uint8_t, 32>>& replaced_txids
) const {
    uint64_t total_fees = 0;
    
    for (const auto& txid : replaced_txids) {
        auto it = transactions_.find(txid);
        if (it != transactions_.end()) {
            total_fees += it->second.fee;
        }
    }
    
    return total_fees;
}

// CPFP: Child-Pays-For-Parent implementation
void Mempool::UpdateRelationships(const std::array<uint8_t, 32>& txid) {
    auto it = transactions_.find(txid);
    if (it == transactions_.end()) {
        return;
    }
    
    const auto& tx = it->second.tx;
    
    // Find parent transactions (those whose outputs this tx spends)
    for (const auto& input : tx.inputs) {
        auto parent_it = transactions_.find(input.prevout.txid);
        if (parent_it != transactions_.end()) {
            // This is a child of parent_it
            children_[input.prevout.txid].push_back(txid);
            parents_[txid].push_back(input.prevout.txid);
        }
    }
}

void Mempool::RemoveFromRelationships(const std::array<uint8_t, 32>& txid) {
    // Remove from children_ map
    children_.erase(txid);
    
    // Remove from parents_ lists
    for (auto& [parent_id, children] : children_) {
        children.erase(
            std::remove(children.begin(), children.end(), txid),
            children.end()
        );
    }
    
    // Remove from parents_ map
    auto it = parents_.find(txid);
    if (it != parents_.end()) {
        // Remove this child from all parents
        for (const auto& parent_id : it->second) {
            auto parent_children_it = children_.find(parent_id);
            if (parent_children_it != children_.end()) {
                auto& children = parent_children_it->second;
                children.erase(
                    std::remove(children.begin(), children.end(), txid),
                    children.end()
                );
            }
        }
        parents_.erase(it);
    }
}

void Mempool::UpdateAncestorState(const std::array<uint8_t, 32>& txid) {
    auto it = transactions_.find(txid);
    if (it == transactions_.end()) {
        return;
    }
    
    // Calculate ancestor fee and size
    uint64_t total_fee = it->second.fee;
    size_t total_size = it->second.size;
    uint32_t ancestor_count = 0;
    
    // Find all parent transactions in mempool
    auto parent_it = parents_.find(txid);
    if (parent_it != parents_.end()) {
        for (const auto& parent_id : parent_it->second) {
            auto parent_entry_it = transactions_.find(parent_id);
            if (parent_entry_it != transactions_.end()) {
                total_fee += parent_entry_it->second.ancestor_fee;
                total_size += parent_entry_it->second.ancestor_size;
                ancestor_count += parent_entry_it->second.ancestor_count + 1;
            }
        }
    }
    
    // Update entry (need to remove and re-add to update priority queue)
    auto entry = it->second;
    priority_queue_.erase(entry);
    
    entry.ancestor_fee = total_fee;
    entry.ancestor_size = total_size;
    entry.ancestor_count = ancestor_count;
    
    transactions_[txid] = entry;
    priority_queue_.insert(entry);
    
    // Recursively update descendants
    auto children_it = children_.find(txid);
    if (children_it != children_.end()) {
        for (const auto& child_id : children_it->second) {
            UpdateAncestorState(child_id);
        }
    }
}

std::vector<std::array<uint8_t, 32>> Mempool::GetDescendants(
    const std::array<uint8_t, 32>& txid
) const {
    std::vector<std::array<uint8_t, 32>> descendants;
    std::vector<std::array<uint8_t, 32>> to_process = {txid};
    
    while (!to_process.empty()) {
        auto current = to_process.back();
        to_process.pop_back();
        
        auto it = children_.find(current);
        if (it != children_.end()) {
            for (const auto& child : it->second) {
                descendants.push_back(child);
                to_process.push_back(child);
            }
        }
    }
    
    return descendants;
}

std::vector<std::vector<primitives::Transaction>> Mempool::GetTransactionPackages(
    size_t max_count
) const {
    std::vector<std::vector<primitives::Transaction>> packages;
    std::set<std::array<uint8_t, 32>> processed;
    
    // Iterate through priority queue (highest fee rate first)
    for (const auto& entry : priority_queue_) {
        if (packages.size() >= max_count) {
            break;
        }
        
        auto txid = entry.tx.GetTxID();
        if (processed.find(txid) != processed.end()) {
            continue; // Already included in a package
        }
        
        // Build package: this transaction + all ancestors + all descendants
        std::vector<primitives::Transaction> package;
        std::set<std::array<uint8_t, 32>> package_txids;
        
        // Add transaction itself
        package.push_back(entry.tx);
        package_txids.insert(txid);
        
        // Add ancestors
        auto parent_it = parents_.find(txid);
        if (parent_it != parents_.end()) {
            for (const auto& parent_id : parent_it->second) {
                if (package_txids.find(parent_id) == package_txids.end()) {
                    auto parent_entry_it = transactions_.find(parent_id);
                    if (parent_entry_it != transactions_.end()) {
                        package.push_back(parent_entry_it->second.tx);
                        package_txids.insert(parent_id);
                    }
                }
            }
        }
        
        // Add descendants
        auto descendants = GetDescendants(txid);
        for (const auto& desc_id : descendants) {
            if (package_txids.find(desc_id) == package_txids.end()) {
                auto desc_entry_it = transactions_.find(desc_id);
                if (desc_entry_it != transactions_.end()) {
                    package.push_back(desc_entry_it->second.tx);
                    package_txids.insert(desc_id);
                }
            }
        }
        
        // Mark all transactions in package as processed
        for (const auto& pkg_txid : package_txids) {
            processed.insert(pkg_txid);
        }
        
        packages.push_back(package);
    }
    
    return packages;
}

} // namespace mempool
} // namespace parthenon
