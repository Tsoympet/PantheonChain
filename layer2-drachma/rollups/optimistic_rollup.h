#ifndef PARTHENON_LAYER2_ROLLUPS_OPTIMISTIC_ROLLUP_H
#define PARTHENON_LAYER2_ROLLUPS_OPTIMISTIC_ROLLUP_H

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace layer2 {
namespace rollups {

/**
 * Rollup Batch
 * A batch of transactions processed together
 */
struct RollupBatch {
    uint64_t batch_id;
    std::array<uint8_t, 32> state_root_before;
    std::array<uint8_t, 32> state_root_after;
    std::vector<std::array<uint8_t, 32>> transactions;
    uint64_t timestamp;
    std::vector<uint8_t> operator_signature;

    RollupBatch() : batch_id(0), timestamp(0) {
        state_root_before.fill(0);
        state_root_after.fill(0);
    }
};

/**
 * Rollup Transaction
 * Transaction in the rollup
 */
struct RollupTx {
    std::array<uint8_t, 32> tx_hash;
    std::vector<uint8_t> from;
    std::vector<uint8_t> to;
    uint64_t value;
    uint64_t nonce;
    std::vector<uint8_t> data;
    std::vector<uint8_t> signature;

    RollupTx() : value(0), nonce(0) { tx_hash.fill(0); }
};

/**
 * Fraud Proof
 * Proof that a batch was processed incorrectly
 */
struct FraudProof {
    uint64_t batch_id;
    uint64_t disputed_tx_index;
    std::array<uint8_t, 32> claimed_state_root;
    std::array<uint8_t, 32> correct_state_root;
    std::vector<uint8_t> state_proof_before;
    std::vector<uint8_t> state_proof_after;
    std::vector<uint8_t> witness_data;

    FraudProof() : batch_id(0), disputed_tx_index(0) {
        claimed_state_root.fill(0);
        correct_state_root.fill(0);
    }
};

/**
 * Optimistic Rollup
 * Batch transaction processing with fraud proofs
 */
class OptimisticRollup {
  public:
    OptimisticRollup();
    ~OptimisticRollup();

    /**
     * Submit a new batch
     */
    bool SubmitBatch(const RollupBatch& batch);

    /**
     * Get batch by ID
     */
    std::optional<RollupBatch> GetBatch(uint64_t batch_id) const;

    /**
     * Add transaction to pending batch
     */
    bool AddTransaction(const RollupTx& tx);

    /**
     * Create batch from pending transactions
     */
    RollupBatch CreateBatch();

    /**
     * Submit fraud proof
     */
    bool SubmitFraudProof(const FraudProof& proof);

    /**
     * Verify fraud proof
     */
    bool VerifyFraudProof(const FraudProof& proof) const;

    /**
     * Finalize batch after challenge period
     */
    bool FinalizeBatch(uint64_t batch_id);

    /**
     * Get pending batches
     */
    std::vector<RollupBatch> GetPendingBatches() const;

    /**
     * Set challenge period in blocks
     */
    void SetChallengePeriod(uint64_t blocks) { challenge_period_ = blocks; }

    /**
     * Get challenge period
     */
    uint64_t GetChallengePeriod() const { return challenge_period_; }

    /**
     * Get current batch ID
     */
    uint64_t GetCurrentBatchId() const { return current_batch_id_; }

    /**
     * Compress batch data
     */
    std::vector<uint8_t> CompressBatch(const RollupBatch& batch) const;

    /**
     * Decompress batch data
     */
    std::optional<RollupBatch> DecompressBatch(const std::vector<uint8_t>& data) const;

  private:
    struct BatchInfo {
        RollupBatch batch;
        uint64_t submission_block;
        bool finalized;
        bool challenged;
    };

    uint64_t current_batch_id_;
    uint64_t challenge_period_;
    uint64_t current_block_height_;
    std::array<uint8_t, 32> current_state_root_;
    std::map<uint64_t, BatchInfo> batches_;
    std::vector<RollupTx> pending_transactions_;
};

/**
 * Rollup Sequencer
 * Sequences and batches transactions
 */
class RollupSequencer {
  public:
    explicit RollupSequencer(OptimisticRollup* rollup);
    ~RollupSequencer();

    /**
     * Process pending transactions
     */
    RollupBatch ProcessPendingTransactions();

    /**
     * Validate transaction
     */
    bool ValidateTransaction(const RollupTx& tx) const;

    /**
     * Calculate new state root
     */
    std::array<uint8_t, 32> CalculateStateRoot(const std::array<uint8_t, 32>& prev_root,
                                               const std::vector<RollupTx>& transactions) const;

    /**
     * Set maximum batch size
     */
    void SetMaxBatchSize(size_t size) { max_batch_size_ = size; }

    /**
     * Get maximum batch size
     */
    size_t GetMaxBatchSize() const { return max_batch_size_; }

  private:
    OptimisticRollup* rollup_;
    size_t max_batch_size_;
};

/**
 * Rollup Verifier
 * Verifies rollup batches and generates fraud proofs
 */
class RollupVerifier {
  public:
    explicit RollupVerifier(OptimisticRollup* rollup);
    ~RollupVerifier();

    /**
     * Verify batch is correct
     */
    bool VerifyBatch(const RollupBatch& batch) const;

    /**
     * Generate fraud proof if batch is invalid
     */
    std::optional<FraudProof> GenerateFraudProof(uint64_t batch_id) const;

    /**
     * Re-execute transaction
     */
    std::array<uint8_t, 32> ReExecuteTransaction(const RollupTx& tx,
                                                 const std::array<uint8_t, 32>& state_root) const;

  private:
    OptimisticRollup* rollup_;
};

}  // namespace rollups
}  // namespace layer2
}  // namespace parthenon

#endif  // PARTHENON_LAYER2_ROLLUPS_OPTIMISTIC_ROLLUP_H
