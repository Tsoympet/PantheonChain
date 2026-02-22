#ifndef PARTHENON_LAYER2_PLASMA_PLASMA_CHAIN_H
#define PARTHENON_LAYER2_PLASMA_PLASMA_CHAIN_H

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace parthenon {
namespace layer2 {
namespace plasma {

/**
 * Plasma Block
 * Represents a block in the Plasma chain
 */
struct PlasmaBlock {
    std::array<uint8_t, 32> block_hash;
    std::array<uint8_t, 32> prev_hash;
    std::array<uint8_t, 32> merkle_root;
    uint64_t block_number;
    uint64_t timestamp;
    std::vector<std::array<uint8_t, 32>> transactions;

    PlasmaBlock() : block_number(0), timestamp(0) {
        block_hash.fill(0);
        prev_hash.fill(0);
        merkle_root.fill(0);
    }
};

/**
 * Plasma Transaction
 * Transaction on the Plasma chain
 */
struct PlasmaTx {
    std::array<uint8_t, 32> tx_hash;
    std::vector<uint8_t> sender;
    std::vector<uint8_t> recipient;
    uint64_t amount;
    uint64_t nonce;
    std::vector<uint8_t> signature;

    PlasmaTx() : amount(0), nonce(0) { tx_hash.fill(0); }
};

/**
 * Exit Request
 * Request to exit from Plasma chain to main chain
 */
struct ExitRequest {
    std::array<uint8_t, 32> tx_hash;
    uint64_t plasma_block_number;
    std::vector<uint8_t> owner;
    uint64_t amount;
    std::vector<uint8_t> merkle_proof;
    uint64_t challenge_period_end;
    bool challenged;

    ExitRequest() : plasma_block_number(0), amount(0), challenge_period_end(0), challenged(false) { tx_hash.fill(0); }
};

/**
 * Plasma Chain
 * Manages a Layer 2 Plasma chain for scaling
 */
class PlasmaChain {
  public:
    PlasmaChain();
    ~PlasmaChain();

    /**
     * Submit a new Plasma block to main chain
     */
    bool SubmitBlock(const PlasmaBlock& block);

    /**
     * Get Plasma block by number
     */
    std::optional<PlasmaBlock> GetBlock(uint64_t block_number) const;

    /**
     * Add transaction to current Plasma block
     */
    bool AddTransaction(const PlasmaTx& tx);

    /**
     * Create Merkle tree for transactions
     */
    std::array<uint8_t, 32>
    BuildMerkleRoot(const std::vector<std::array<uint8_t, 32>>& tx_hashes) const;

    /**
     * Request exit from Plasma chain
     */
    bool RequestExit(const ExitRequest& request);

    /**
     * Challenge an exit request
     */
    bool ChallengeExit(const std::array<uint8_t, 32>& tx_hash,
                       const std::vector<uint8_t>& fraud_proof);

    /**
     * Finalize exit after challenge period
     */
    bool FinalizeExit(const std::array<uint8_t, 32>& tx_hash);

    /**
     * Get pending exit requests
     */
    std::vector<ExitRequest> GetPendingExits() const;

    /**
     * Verify Merkle proof
     */
    bool VerifyMerkleProof(const std::array<uint8_t, 32>& tx_hash,
                           const std::array<uint8_t, 32>& merkle_root,
                           const std::vector<uint8_t>& proof) const;

    /**
     * Get current block number
     */
    uint64_t GetCurrentBlockNumber() const { return current_block_number_; }

    /**
     * Set challenge period in blocks
     */
    void SetChallengePeriod(uint64_t blocks) { challenge_period_ = blocks; }

    /**
     * Get challenge period
     */
    uint64_t GetChallengePeriod() const { return challenge_period_; }

  private:
    uint64_t current_block_number_;
    uint64_t challenge_period_;  // Challenge period in blocks
    std::map<uint64_t, PlasmaBlock> blocks_;
    std::map<std::array<uint8_t, 32>, ExitRequest> exit_requests_;
    std::vector<PlasmaTx> pending_transactions_;
};

/**
 * Plasma Operator
 * Manages Plasma chain operations
 */
class PlasmaOperator {
  public:
    explicit PlasmaOperator(PlasmaChain* chain);
    ~PlasmaOperator();

    /**
     * Create new Plasma block
     */
    PlasmaBlock CreateBlock();

    /**
     * Validate Plasma transaction
     */
    bool ValidateTransaction(const PlasmaTx& tx) const;

    /**
     * Process exit request
     */
    bool ProcessExitRequest(const ExitRequest& request);

  private:
    PlasmaChain* chain_;
};

}  // namespace plasma
}  // namespace layer2
}  // namespace parthenon

#endif  // PARTHENON_LAYER2_PLASMA_PLASMA_CHAIN_H
