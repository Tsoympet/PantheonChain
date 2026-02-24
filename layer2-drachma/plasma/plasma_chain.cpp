#include "plasma_chain.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace layer2 {
namespace plasma {

// PlasmaChain Implementation
PlasmaChain::PlasmaChain()
    : current_block_number_(0),
      challenge_period_(100)  // Default 100 blocks
{}

PlasmaChain::~PlasmaChain() = default;

bool PlasmaChain::SubmitBlock(const PlasmaBlock& block) {
    // Verify block number is sequential
    if (block.block_number != current_block_number_ + 1) {
        return false;
    }

    // Verify previous hash matches
    if (current_block_number_ > 0) {
        auto prev_it = blocks_.find(current_block_number_);
        if (prev_it != blocks_.end()) {
            if (block.prev_hash != prev_it->second.block_hash) {
                return false;
            }
        }
    }

    // Store the block
    blocks_[block.block_number] = block;
    current_block_number_ = block.block_number;

    return true;
}

std::optional<PlasmaBlock> PlasmaChain::GetBlock(uint64_t block_number) const {
    auto it = blocks_.find(block_number);
    if (it == blocks_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool PlasmaChain::AddTransaction(const PlasmaTx& tx) {
    // Basic validation
    if (tx.sender.empty() || tx.recipient.empty()) {
        return false;
    }

    if (tx.amount == 0) {
        return false;
    }

    // Add to pending transactions
    pending_transactions_.push_back(tx);

    return true;
}

std::array<uint8_t, 32>
PlasmaChain::BuildMerkleRoot(const std::vector<std::array<uint8_t, 32>>& tx_hashes) const {
    std::array<uint8_t, 32> root;
    root.fill(0);

    if (tx_hashes.empty()) {
        return root;
    }

    // Merkle tree bottom-up construction using SHA256(left || right)
    std::vector<std::array<uint8_t, 32>> current_level = tx_hashes;

    while (current_level.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next_level;

        for (size_t i = 0; i < current_level.size(); i += 2) {
            const std::array<uint8_t, 32>& left = current_level[i];
            const std::array<uint8_t, 32>& right =
                (i + 1 < current_level.size()) ? current_level[i + 1] : current_level[i];

            crypto::SHA256 hasher;
            hasher.Write(left.data(), left.size());
            hasher.Write(right.data(), right.size());
            next_level.push_back(hasher.Finalize());
        }

        current_level = next_level;
    }

    return current_level[0];
}

bool PlasmaChain::RequestExit(const ExitRequest& request) {
    // Verify the Plasma block exists
    auto block = GetBlock(request.plasma_block_number);
    if (!block) {
        return false;
    }

    // Verify Merkle proof
    if (!VerifyMerkleProof(request.tx_hash, block->merkle_root, request.merkle_proof)) {
        return false;
    }

    // Create exit request with challenge period
    ExitRequest exit = request;
    exit.challenge_period_end = current_block_number_ + challenge_period_;
    exit.challenged = false;

    exit_requests_[request.tx_hash] = exit;

    return true;
}

bool PlasmaChain::ChallengeExit(const std::array<uint8_t, 32>& tx_hash,
                                const std::vector<uint8_t>& fraud_proof) {
    auto it = exit_requests_.find(tx_hash);
    if (it == exit_requests_.end()) {
        return false;
    }

    // Verify challenge is within challenge period
    if (current_block_number_ >= it->second.challenge_period_end) {
        return false;
    }

    // Verify fraud proof: must be non-empty and must contain the 32-byte tx_hash
    // of the challenged exit as its first 32 bytes, proving the challenger is
    // referencing a specific transaction rather than submitting a generic blob.
    // The bytes after the first 32 should carry a Merkle inclusion proof of a
    // conflicting spend in the same slot (33 bytes each: direction || sibling_hash),
    // structured identically to the proofs accepted by VerifyMerkleProof().
    // Full re-execution of the conflicting spend against the referenced Plasma block
    // is required before this function can be considered production-complete.
    if (fraud_proof.size() < 32) {
        return false;
    }
    std::array<uint8_t, 32> proof_tx_hash;
    std::memcpy(proof_tx_hash.data(), fraud_proof.data(), 32);
    if (proof_tx_hash != tx_hash) {
        return false;
    }

    // Mark as challenged
    it->second.challenged = true;

    return true;
}

bool PlasmaChain::FinalizeExit(const std::array<uint8_t, 32>& tx_hash) {
    auto it = exit_requests_.find(tx_hash);
    if (it == exit_requests_.end()) {
        return false;
    }

    // Verify challenge period has ended
    if (current_block_number_ < it->second.challenge_period_end) {
        return false;
    }

    // Verify not challenged
    if (it->second.challenged) {
        // Exit was successfully challenged, reject
        exit_requests_.erase(it);
        return false;
    }

    // Exit is valid, process withdrawal
    // This would transfer funds on main chain
    exit_requests_.erase(it);

    return true;
}

std::vector<ExitRequest> PlasmaChain::GetPendingExits() const {
    std::vector<ExitRequest> result;
    result.reserve(exit_requests_.size());

    for (const auto& [hash, request] : exit_requests_) {
        result.push_back(request);
    }

    return result;
}

bool PlasmaChain::VerifyMerkleProof(const std::array<uint8_t, 32>& tx_hash,
                                    const std::array<uint8_t, 32>& merkle_root,
                                    const std::vector<uint8_t>& proof) const {
    // Each proof element is 33 bytes: 1 byte direction + 32 bytes sibling hash.
    // Direction: 0 = sibling is on the right (tx is left child),
    //            1 = sibling is on the left  (tx is right child).
    if (proof.empty() || proof.size() % 33 != 0) {
        return false;
    }

    std::array<uint8_t, 32> current = tx_hash;
    size_t num_levels = proof.size() / 33;

    for (size_t i = 0; i < num_levels; ++i) {
        uint8_t direction = proof[i * 33];
        std::array<uint8_t, 32> sibling;
        std::memcpy(sibling.data(), proof.data() + i * 33 + 1, 32);

        crypto::SHA256 hasher;
        if (direction == 0) {
            // current is the left child, sibling is the right child
            hasher.Write(current.data(), current.size());
            hasher.Write(sibling.data(), sibling.size());
        } else {
            // sibling is the left child, current is the right child
            hasher.Write(sibling.data(), sibling.size());
            hasher.Write(current.data(), current.size());
        }
        current = hasher.Finalize();
    }

    return current == merkle_root;
}

// PlasmaOperator Implementation
PlasmaOperator::PlasmaOperator(PlasmaChain* chain) : chain_(chain) {}

PlasmaOperator::~PlasmaOperator() = default;

PlasmaBlock PlasmaOperator::CreateBlock() {
    PlasmaBlock block;

    // Set block number
    block.block_number = chain_->GetCurrentBlockNumber() + 1;

    // Set timestamp (would use actual time in production)
    block.timestamp = block.block_number * 1000;

    // Set previous hash
    if (block.block_number > 1) {
        auto prev = chain_->GetBlock(block.block_number - 1);
        if (prev) {
            block.prev_hash = prev->block_hash;
        }
    }

    // Build Merkle root from transactions
    block.merkle_root = chain_->BuildMerkleRoot(block.transactions);

    // Compute block hash from block data
    crypto::SHA256 hasher;
    hasher.Write(reinterpret_cast<const uint8_t*>(&block.block_number), sizeof(uint64_t));
    hasher.Write(block.prev_hash.data(), block.prev_hash.size());
    hasher.Write(block.merkle_root.data(), block.merkle_root.size());
    hasher.Write(reinterpret_cast<const uint8_t*>(&block.timestamp), sizeof(uint64_t));
    block.block_hash = hasher.Finalize();

    return block;
}

bool PlasmaOperator::ValidateTransaction(const PlasmaTx& tx) const {
    // Verify sender and recipient
    if (tx.sender.empty() || tx.recipient.empty()) {
        return false;
    }

    // Verify amount
    if (tx.amount == 0) {
        return false;
    }

    // Verify signature exists
    if (tx.signature.empty()) {
        return false;
    }

    return true;
}

bool PlasmaOperator::ProcessExitRequest(const ExitRequest& request) {
    return chain_->RequestExit(request);
}

}  // namespace plasma
}  // namespace layer2
}  // namespace parthenon
