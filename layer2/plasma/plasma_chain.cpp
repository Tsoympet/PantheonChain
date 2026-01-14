#include "plasma_chain.h"

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

    // Simple Merkle root calculation
    // In production, this would use a proper Merkle tree implementation
    std::vector<std::array<uint8_t, 32>> current_level = tx_hashes;

    while (current_level.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next_level;

        for (size_t i = 0; i < current_level.size(); i += 2) {
            std::array<uint8_t, 32> combined;

            if (i + 1 < current_level.size()) {
                // Hash two nodes together - copy first 16 bytes from each
                std::memcpy(combined.data(), current_level[i].data(), 16);
                std::memcpy(combined.data() + 16, current_level[i + 1].data(), 16);
            } else {
                // Odd number, duplicate last node
                combined = current_level[i];
            }

            next_level.push_back(combined);
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

    // Verify fraud proof (simplified)
    if (fraud_proof.empty()) {
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
    // Simplified Merkle proof verification
    if (proof.empty()) {
        return false;
    }

    // In production, this would properly verify the Merkle path
    // For now, basic validation
    return true;
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

    // Calculate block hash (simplified)
    
    // Calculate block hash (simplified) - use only 8 bytes to avoid overflow
    std::memset(block.block_hash.data(), 0, 32);  // Zero the entire hash first
    std::memcpy(block.block_hash.data(), &block.block_number, sizeof(uint64_t));

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
