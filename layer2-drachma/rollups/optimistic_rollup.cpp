#include "optimistic_rollup.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace layer2 {
namespace rollups {

// OptimisticRollup Implementation
OptimisticRollup::OptimisticRollup()
    : current_batch_id_(0), challenge_period_(100), current_block_height_(0) {
    current_state_root_.fill(0);
}

OptimisticRollup::~OptimisticRollup() = default;

bool OptimisticRollup::SubmitBatch(const RollupBatch& batch) {
    // Verify batch ID is sequential
    if (batch.batch_id != current_batch_id_ + 1) {
        return false;
    }

    // Verify state root continuity
    if (current_batch_id_ > 0 && batch.state_root_before != current_state_root_) {
        return false;
    }

    // Store batch with metadata
    BatchInfo info;
    info.batch = batch;
    info.submission_block = current_block_height_;
    info.finalized = false;
    info.challenged = false;

    batches_[batch.batch_id] = info;
    current_batch_id_ = batch.batch_id;
    current_state_root_ = batch.state_root_after;

    return true;
}

std::optional<RollupBatch> OptimisticRollup::GetBatch(uint64_t batch_id) const {
    auto it = batches_.find(batch_id);
    if (it == batches_.end()) {
        return std::nullopt;
    }
    return it->second.batch;
}

bool OptimisticRollup::AddTransaction(const RollupTx& tx) {
    // Basic validation
    if (tx.from.empty() || tx.to.empty()) {
        return false;
    }

    if (tx.signature.empty()) {
        return false;
    }

    pending_transactions_.push_back(tx);
    return true;
}

RollupBatch OptimisticRollup::CreateBatch() {
    RollupBatch batch;
    batch.batch_id = current_batch_id_ + 1;
    batch.state_root_before = current_state_root_;
    batch.timestamp = current_block_height_;

    // Add pending transactions
    batch.transactions.reserve(pending_transactions_.size());
    for (const auto& tx : pending_transactions_) {
        batch.transactions.push_back(tx.tx_hash);
    }

    // Clear pending
    pending_transactions_.clear();

    return batch;
}

bool OptimisticRollup::SubmitFraudProof(const FraudProof& proof) {
    auto it = batches_.find(proof.batch_id);
    if (it == batches_.end()) {
        return false;
    }

    // Verify batch is not finalized
    if (it->second.finalized) {
        return false;
    }

    // Verify fraud proof
    if (!VerifyFraudProof(proof)) {
        return false;
    }

    // Mark batch as challenged
    it->second.challenged = true;

    // Rollback state
    if (it->second.batch.batch_id == current_batch_id_) {
        current_state_root_ = it->second.batch.state_root_before;
        current_batch_id_--;
    }

    return true;
}

bool OptimisticRollup::VerifyFraudProof(const FraudProof& proof) const {
    // Verify the batch exists
    auto it = batches_.find(proof.batch_id);
    if (it == batches_.end()) {
        return false;
    }

    // Verify transaction index is valid
    if (proof.disputed_tx_index >= it->second.batch.transactions.size()) {
        return false;
    }

    // Verify state roots don't match
    if (proof.claimed_state_root == proof.correct_state_root) {
        return false;
    }

    // Verify witness data is provided
    if (proof.witness_data.empty()) {
        return false;
    }

    // Additional verification would happen here
    return true;
}

bool OptimisticRollup::FinalizeBatch(uint64_t batch_id) {
    auto it = batches_.find(batch_id);
    if (it == batches_.end()) {
        return false;
    }

    // Verify challenge period has passed
    if (current_block_height_ < it->second.submission_block + challenge_period_) {
        return false;
    }

    // Verify not challenged
    if (it->second.challenged) {
        return false;
    }

    // Finalize
    it->second.finalized = true;

    return true;
}

std::vector<RollupBatch> OptimisticRollup::GetPendingBatches() const {
    std::vector<RollupBatch> result;

    for (const auto& [id, info] : batches_) {
        if (!info.finalized) {
            result.push_back(info.batch);
        }
    }

    return result;
}

std::vector<uint8_t> OptimisticRollup::CompressBatch(const RollupBatch& batch) const {
    std::vector<uint8_t> compressed;

    // Simple compression: serialize batch state roots and transaction data
    compressed.reserve(64 + batch.transactions.size() * 32);

    // Add state roots
    compressed.insert(compressed.end(), batch.state_root_before.begin(),
                      batch.state_root_before.end());
    compressed.insert(compressed.end(), batch.state_root_after.begin(),
                      batch.state_root_after.end());

    // Add transaction count
    uint32_t tx_count = static_cast<uint32_t>(batch.transactions.size());
    const uint8_t* count_bytes = reinterpret_cast<const uint8_t*>(&tx_count);
    compressed.insert(compressed.end(), count_bytes, count_bytes + sizeof(uint32_t));

    // Add transaction hashes
    for (const auto& tx_hash : batch.transactions) {
        compressed.insert(compressed.end(), tx_hash.begin(), tx_hash.end());
    }

    return compressed;
}

std::optional<RollupBatch>
OptimisticRollup::DecompressBatch(const std::vector<uint8_t>& data) const {
    if (data.size() < 68) {  // 32 + 32 + 4 minimum
        return std::nullopt;
    }

    RollupBatch batch;
    size_t offset = 0;

    // Read state roots
    std::memcpy(batch.state_root_before.data(), data.data() + offset, 32);
    offset += 32;
    std::memcpy(batch.state_root_after.data(), data.data() + offset, 32);
    offset += 32;

    // Read transaction count
    uint32_t tx_count;
    std::memcpy(&tx_count, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Read transaction hashes
    batch.transactions.reserve(tx_count);
    for (uint32_t i = 0; i < tx_count; ++i) {
        if (offset + 32 > data.size()) {
            return std::nullopt;
        }

        std::array<uint8_t, 32> tx_hash;
        std::memcpy(tx_hash.data(), data.data() + offset, 32);
        batch.transactions.push_back(tx_hash);
        offset += 32;
    }

    return batch;
}

// RollupSequencer Implementation
RollupSequencer::RollupSequencer(OptimisticRollup* rollup)
    : rollup_(rollup), max_batch_size_(1000) {}

RollupSequencer::~RollupSequencer() = default;

RollupBatch RollupSequencer::ProcessPendingTransactions() {
    return rollup_->CreateBatch();
}

bool RollupSequencer::ValidateTransaction(const RollupTx& tx) const {
    if (tx.from.empty() || tx.to.empty()) {
        return false;
    }

    if (tx.signature.empty()) {
        return false;
    }

    return true;
}

std::array<uint8_t, 32>
RollupSequencer::CalculateStateRoot(const std::array<uint8_t, 32>& prev_root,
                                    const std::vector<RollupTx>& transactions) const {
    // Compute new state root as SHA256(prev_root || tx_hash_0 || tx_hash_1 || ...)
    crypto::SHA256 hasher;
    hasher.Write(prev_root.data(), prev_root.size());
    for (const auto& tx : transactions) {
        hasher.Write(tx.tx_hash.data(), tx.tx_hash.size());
    }
    return hasher.Finalize();
}

// RollupVerifier Implementation
RollupVerifier::RollupVerifier(OptimisticRollup* rollup) : rollup_(rollup) {}

RollupVerifier::~RollupVerifier() = default;

bool RollupVerifier::VerifyBatch(const RollupBatch& batch) const {
    // Verify batch has transactions
    if (batch.transactions.empty()) {
        return false;
    }

    // Verify state roots are different (some change occurred)
    // This is a simplified check
    bool roots_differ = false;
    for (size_t i = 0; i < 32; ++i) {
        if (batch.state_root_before[i] != batch.state_root_after[i]) {
            roots_differ = true;
            break;
        }
    }

    if (!roots_differ) {
        return false;
    }

    // Verify signature exists
    if (batch.operator_signature.empty()) {
        return false;
    }

    return true;
}

std::optional<FraudProof> RollupVerifier::GenerateFraudProof(uint64_t batch_id) const {
    auto batch = rollup_->GetBatch(batch_id);
    if (!batch) {
        return std::nullopt;
    }

    // Verify the batch
    if (VerifyBatch(*batch)) {
        // No fraud detected
        return std::nullopt;
    }

    // Generate fraud proof
    FraudProof proof;
    proof.batch_id = batch_id;
    proof.disputed_tx_index = 0;
    proof.claimed_state_root = batch->state_root_after;

    // Would calculate correct state root here
    proof.correct_state_root.fill(0);

    return proof;
}

std::array<uint8_t, 32>
RollupVerifier::ReExecuteTransaction(const RollupTx& tx,
                                     const std::array<uint8_t, 32>& state_root) const {
    // Apply transaction to state: new_root = SHA256(state_root || tx_hash)
    crypto::SHA256 hasher;
    hasher.Write(state_root.data(), state_root.size());
    hasher.Write(tx.tx_hash.data(), tx.tx_hash.size());
    return hasher.Finalize();
}

}  // namespace rollups
}  // namespace layer2
}  // namespace parthenon
