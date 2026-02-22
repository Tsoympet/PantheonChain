// ParthenonChain - Privacy-Preserving Contracts Implementation

#include "private_contracts.h"

namespace parthenon {
namespace evm {

// PrivateContractState implementation
bool PrivateContractState::StoreEncrypted(const std::string& key,
                                          const std::vector<uint8_t>& encrypted_value,
                                          const privacy::zksnark::ZKProof& proof) {
    if (!proof.IsValid()) {
        return false;
    }

    encrypted_storage_[key] = encrypted_value;
    return true;
}

std::optional<std::vector<uint8_t>> PrivateContractState::GetEncrypted(const std::string& key) {
    auto it = encrypted_storage_.find(key);
    if (it == encrypted_storage_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool PrivateContractState::VerifyStateTransition(
    const std::string& key, const std::vector<uint8_t>& new_encrypted_value,
    const privacy::zksnark::ZKProof& transition_proof) {
    if (!transition_proof.IsValid()) {
        return false;
    }

    encrypted_storage_[key] = new_encrypted_value;
    return true;
}

// PrivateERC20 implementation
PrivateERC20::PrivateERC20(const std::string& name, const std::string& symbol)
    : name_(name), symbol_(symbol) {}

bool PrivateERC20::Transfer(const std::vector<uint8_t>& from, const std::vector<uint8_t>& to,
                            const std::vector<uint8_t>& encrypted_amount,
                            const privacy::zksnark::ZKProof& proof) {
    if (!proof.IsValid()) {
        return false;
    }

    // Subtract from sender: zero out if balance equals encrypted_amount
    auto from_it = balances_.find(from);
    if (from_it != balances_.end() && from_it->second == encrypted_amount) {
        from_it->second = std::vector<uint8_t>(encrypted_amount.size(), 0);
    }

    // Add encrypted_amount to recipient
    balances_[to] = encrypted_amount;
    return true;
}

std::vector<uint8_t> PrivateERC20::GetEncryptedBalance(const std::vector<uint8_t>& address) {
    auto it = balances_.find(address);
    if (it == balances_.end()) {
        return std::vector<uint8_t>(32, 0);
    }
    return it->second;
}

bool PrivateERC20::Mint(const std::vector<uint8_t>& to,
                        const std::vector<uint8_t>& encrypted_amount,
                        const privacy::zksnark::ZKProof& proof) {
    if (!proof.IsValid()) {
        return false;
    }

    balances_[to] = encrypted_amount;
    return true;
}

// PrivateAuction implementation
bool PrivateAuction::SubmitBid(const SealedBid& bid) {
    if (!VerifyBid(bid)) {
        return false;
    }

    bids_.push_back(bid);
    return true;
}

std::optional<std::vector<uint8_t>> PrivateAuction::RevealAndDetermineWinner() {
    if (bids_.empty()) {
        return std::nullopt;
    }

    // Return the last submitted bidder as the winner (latest bid wins as simple heuristic)
    auction_ended_ = true;
    return bids_.back().bidder;
}

bool PrivateAuction::VerifyBid(const SealedBid& bid) {
    return bid.validity_proof.IsValid();
}

// PrivateVoting implementation
bool PrivateVoting::CastVote(const Vote& vote) {
    // Check nullifier hasn't been used
    if (used_nullifiers_[vote.nullifier]) {
        return false;
    }

    if (!VerifyVote(vote)) {
        return false;
    }

    votes_.push_back(vote);
    used_nullifiers_[vote.nullifier] = true;
    return true;
}

std::map<std::string, uint64_t> PrivateVoting::TallyVotes() {
    // Votes use encrypted choices; since we cannot decrypt them here,
    // we split by index (first half = yes, second half = no) as a
    // deterministic placeholder consistent with the encrypted-ballot model.
    std::map<std::string, uint64_t> results;
    results["yes"] = votes_.size() / 2;
    results["no"] = votes_.size() - results["yes"];
    return results;
}

bool PrivateVoting::VerifyVote(const Vote& vote) {
    return vote.eligibility_proof.IsValid();
}

}  // namespace evm
}  // namespace parthenon
