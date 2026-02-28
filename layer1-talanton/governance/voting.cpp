#include "voting.h"
#include "boule.h"

#include "../core/crypto/schnorr.h"
#include "../core/crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace governance {

// ---------------------------------------------------------------------------
// File-scope helper: decode a uint64_t from 8 bytes stored little-endian.
// ---------------------------------------------------------------------------
static inline uint64_t ReadLE64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v |= static_cast<uint64_t>(p[i]) << (8 * i);
    return v;
}

// VotingSystem Implementation
VotingSystem::VotingSystem()
    : next_proposal_id_(1),
      current_block_height_(0),
      voting_period_(10000)  // Default: 10000 blocks
      ,
      default_quorum_(1000000)  // Default quorum
      ,
      default_threshold_(50)  // 50% approval
      ,
      total_supply_(0),
      veto_threshold_bps_(3334),  // 33.34 % – Cosmos Hub model
      anti_whale_(nullptr),
      boule_(nullptr),
      require_boule_approval_(false),
      snapshot_registry_(nullptr),
      staking_registry_(nullptr),
      gov_params_(nullptr),
      treasury_(nullptr)
{}

VotingSystem::~VotingSystem() = default;

uint64_t VotingSystem::CreateProposal(const std::vector<uint8_t>& proposer, ProposalType type,
                                      const std::string& title, const std::string& description,
                                      const std::vector<uint8_t>& execution_data,
                                      uint64_t deposit_amount) {
    Proposal proposal;
    proposal.proposal_id = next_proposal_id_++;
    proposal.type = type;
    proposal.status = ProposalStatus::PENDING;
    proposal.title = title;
    proposal.description = description;
    proposal.proposer = proposer;
    proposal.creation_time = current_block_height_;
    proposal.voting_start = current_block_height_ + 100;  // 100 block delay
    proposal.voting_end = proposal.voting_start + voting_period_;
    proposal.quorum_requirement = default_quorum_;
    // CONSTITUTIONAL proposals require a higher threshold (≈ 2/3)
    proposal.approval_threshold =
        (type == ProposalType::CONSTITUTIONAL) ? 66 : default_threshold_;
    proposal.execution_data = execution_data;
    proposal.deposit_amount = deposit_amount;
    proposal.deposit_returned = false;
    proposal.boule_approved = !require_boule_approval_;  // pre-approved when screening off

    proposals_[proposal.proposal_id] = proposal;

    // Create a voting-power snapshot at the proposal's voting_start block.
    // This freezes each staker's power so late-staking/flash-stake attacks
    // cannot influence an ongoing vote.
    if (snapshot_registry_ != nullptr && staking_registry_ != nullptr) {
        auto powers = staking_registry_->GetAllVotingPowers();
        snapshot_registry_->CreateSnapshot(proposal.proposal_id,
                                           proposal.voting_start,
                                           powers);
    }

    return proposal.proposal_id;
}

std::optional<Proposal> VotingSystem::GetProposal(uint64_t proposal_id) const {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool VotingSystem::CastVote(uint64_t proposal_id, const std::vector<uint8_t>& voter,
                            VoteChoice choice, uint64_t voting_power,
                            const std::vector<uint8_t>& signature) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }

    Proposal& proposal = it->second;

    // Check Boule approval before allowing votes
    if (require_boule_approval_) {
        bool approved = proposal.boule_approved;
        if (!approved && boule_ != nullptr) {
            approved = boule_->IsProposalApproved(proposal_id);
            if (approved) proposal.boule_approved = true;
        }
        if (!approved) return false;
    }

    // Check voting period
    if (current_block_height_ < proposal.voting_start ||
        current_block_height_ > proposal.voting_end) {
        return false;
    }

    // Check if already voted
    if (HasVoted(proposal_id, voter)) {
        return false;
    }

    // Verify Schnorr signature over canonical vote payload.
    if (signature.size() != crypto::Schnorr::SIGNATURE_SIZE ||
        voter.size() != crypto::Schnorr::PUBLIC_KEY_SIZE) {
        return false;
    }

    std::vector<uint8_t> payload;
    payload.reserve(8 + voter.size() + 1 + 8);

    auto append_u64_le = [&payload](uint64_t value) {
        for (size_t i = 0; i < 8; ++i) {
            payload.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
        }
    };

    append_u64_le(proposal_id);
    payload.insert(payload.end(), voter.begin(), voter.end());
    payload.push_back(static_cast<uint8_t>(choice));
    append_u64_le(voting_power);

    const auto vote_hash = crypto::SHA256::Hash256(payload.data(), payload.size());
    crypto::Schnorr::PublicKey voter_pubkey{};
    std::copy(voter.begin(), voter.end(), voter_pubkey.begin());

    crypto::Schnorr::Signature schnorr_sig{};
    std::copy(signature.begin(), signature.end(), schnorr_sig.begin());

    if (!crypto::Schnorr::Verify(voter_pubkey, vote_hash.data(), schnorr_sig)) {
        return false;
    }

    // If a snapshot exists for this proposal, override the caller-supplied
    // voting_power with the frozen snapshot power.  This prevents a voter from
    // accumulating tokens after the snapshot block to inflate their weight.
    if (snapshot_registry_ != nullptr &&
        snapshot_registry_->HasSnapshot(proposal_id)) {
        voting_power = snapshot_registry_->GetSnapshotPower(proposal_id, voter);
        if (voting_power == 0) {
            return false;  // Voter had no stake at snapshot time
        }
    }

    // Apply anti-whale scaling to raw voting_power before tallying.
    uint64_t effective_power = voting_power;
    if (anti_whale_ != nullptr) {
        effective_power = anti_whale_->ComputeEffectivePower(voting_power, total_supply_);
    }

    // Create vote record (record raw power for auditability)
    Vote vote;
    vote.proposal_id = proposal_id;
    vote.voter = voter;
    vote.choice = choice;
    vote.voting_power = effective_power;
    vote.timestamp = current_block_height_;
    vote.signature = signature;

    // Add to votes
    votes_[proposal_id].push_back(vote);

    // Update tallies using effective (anti-whale-scaled) power
    switch (choice) {
        case VoteChoice::YES:
            proposal.yes_votes += effective_power;
            break;
        case VoteChoice::NO:
            proposal.no_votes += effective_power;
            break;
        case VoteChoice::ABSTAIN:
            proposal.abstain_votes += effective_power;
            break;
        case VoteChoice::VETO:
            proposal.veto_votes += effective_power;
            break;
    }

    // Update status
    if (proposal.status == ProposalStatus::PENDING) {
        proposal.status = ProposalStatus::ACTIVE;
    }

    return true;
}

bool VotingSystem::TallyVotes(uint64_t proposal_id) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }

    Proposal& proposal = it->second;

    // Check voting period has ended
    if (current_block_height_ <= proposal.voting_end) {
        return false;
    }

    // Calculate total votes (all four choices)
    uint64_t total_votes =
        proposal.yes_votes + proposal.no_votes +
        proposal.abstain_votes + proposal.veto_votes;

    // Check quorum
    if (total_votes < proposal.quorum_requirement) {
        proposal.status = ProposalStatus::REJECTED;
        return true;
    }

    // -----------------------------------------------------------------
    // VETO check (Cosmos Hub model):
    // If veto_votes / total_votes > veto_threshold_bps / 10000 then the
    // proposal is REJECTED immediately and the deposit is slashed,
    // regardless of the YES/NO ratio.
    // -----------------------------------------------------------------
    uint64_t effective_threshold =
        (proposal.veto_threshold_bps > 0)
            ? proposal.veto_threshold_bps
            : veto_threshold_bps_;
    // veto_votes / total_votes > effective_threshold / 10000
    // Rearranged: veto_votes * 10000 > total_votes * effective_threshold
    // Use overflow-safe comparison: divide both sides by common factor where possible.
    // Since effective_threshold <= 10000, we compare veto_votes / total_votes
    // using cross-multiplication guarded against uint64_t overflow.
    {
        const uint64_t lhs = (proposal.veto_votes <= UINT64_MAX / 10000ULL)
                                 ? proposal.veto_votes * 10000ULL
                                 : UINT64_MAX;
        const uint64_t rhs = (effective_threshold == 0 || total_votes <= UINT64_MAX / effective_threshold)
                                 ? total_votes * effective_threshold
                                 : UINT64_MAX;
        if (lhs > rhs) {
            proposal.status = ProposalStatus::REJECTED;
            // Mark deposit for slashing (caller invokes SlashDeposit)
            return true;
        }
    }

    // Calculate approval percentage (YES vs YES+NO, excluding ABSTAIN and VETO)
    uint64_t approval_votes = proposal.yes_votes + proposal.no_votes;
    if (approval_votes == 0) {
        proposal.status = ProposalStatus::REJECTED;
        return true;
    }

    uint64_t approval_percent = (proposal.yes_votes <= UINT64_MAX / 100)
                                    ? (proposal.yes_votes * 100) / approval_votes
                                    : 100;  // yes_votes >= approval_votes, so 100%

    // Check if passed
    if (approval_percent >= proposal.approval_threshold) {
        proposal.status = ProposalStatus::PASSED;
        proposal.execution_time = current_block_height_ + 1000;  // Execute after 1000 blocks
    } else {
        proposal.status = ProposalStatus::REJECTED;
    }

    return true;
}

bool VotingSystem::ExecuteProposal(uint64_t proposal_id) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }

    Proposal& proposal = it->second;

    // Check status
    if (proposal.status != ProposalStatus::PASSED) {
        return false;
    }

    // Check execution time
    if (current_block_height_ < proposal.execution_time) {
        return false;
    }

    // Execute proposal based on type.
    // If a custom execution_handler_ is registered, delegate to it.
    if (execution_handler_) {
        if (!execution_handler_(proposal)) {
            return false;  // Handler declined – leave in PASSED state for retry.
        }
    } else {
        // Built-in dispatch: route to the appropriate subsystem based on type.
        switch (proposal.type) {
            case ProposalType::PARAMETER_CHANGE: {
                // execution_data encodes: [key_len(1)] [key_bytes] [value_le8(8)]
                if (gov_params_ != nullptr &&
                    proposal.execution_data.size() >= 10) {
                    uint8_t key_len = proposal.execution_data[0];
                    if (1 + static_cast<size_t>(key_len) + 8 <=
                        proposal.execution_data.size()) {
                        std::string key(
                            reinterpret_cast<const char*>(
                                proposal.execution_data.data() + 1),
                            key_len);
                        uint64_t value = ReadLE64(
                            proposal.execution_data.data() + 1 + key_len);
                        gov_params_->UpdateParam(key, value,
                                                 proposal.proposal_id,
                                                 current_block_height_);
                    }
                }
                break;
            }
            case ProposalType::TREASURY_SPENDING: {
                // execution_data encodes: [amount_le8(8)] [addr_len(1)] [addr_bytes]
                if (treasury_ != nullptr &&
                    proposal.execution_data.size() >= 10) {
                    uint64_t amount = ReadLE64(proposal.execution_data.data());
                    uint8_t addr_len = proposal.execution_data[8];
                    if (9 + static_cast<size_t>(addr_len) <=
                        proposal.execution_data.size() && amount > 0) {
                        std::vector<uint8_t> recipient(
                            proposal.execution_data.begin() + 9,
                            proposal.execution_data.begin() + 9 + addr_len);
                        treasury_->Spend(amount, recipient,
                                         proposal.proposal_id,
                                         Treasury::Track::UNCATEGORIZED,
                                         "proposal execution",
                                         current_block_height_);
                    }
                }
                break;
            }
            default:
                // GENERAL, PROTOCOL_UPGRADE, CONSTITUTIONAL, EMERGENCY:
                // append type tag as audit breadcrumb; caller wires real logic
                // via SetExecutionHandler() for these types.
                proposal.execution_data.push_back(
                    static_cast<uint8_t>(proposal.type));
                break;
        }
    }

    proposal.status = ProposalStatus::EXECUTED;

    return true;
}

std::vector<Proposal> VotingSystem::GetActiveProposals() const {
    std::vector<Proposal> active;

    for (const auto& [id, proposal] : proposals_) {
        if (proposal.status == ProposalStatus::ACTIVE ||
            proposal.status == ProposalStatus::PENDING) {
            active.push_back(proposal);
        }
    }

    return active;
}

std::vector<Vote> VotingSystem::GetProposalVotes(uint64_t proposal_id) const {
    auto it = votes_.find(proposal_id);
    if (it == votes_.end()) {
        return {};
    }
    return it->second;
}

bool VotingSystem::HasVoted(uint64_t proposal_id, const std::vector<uint8_t>& voter) const {
    auto it = votes_.find(proposal_id);
    if (it == votes_.end()) {
        return false;
    }

    for (const auto& vote : it->second) {
        if (vote.voter == voter) {
            return true;
        }
    }

    return false;
}

bool VotingSystem::MarkBouleApproved(uint64_t proposal_id) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) return false;
    it->second.boule_approved = true;
    return true;
}

bool VotingSystem::ReturnDeposit(uint64_t proposal_id) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) return false;
    if (it->second.deposit_returned) return false;
    it->second.deposit_returned = true;
    return true;
}

bool VotingSystem::SlashDeposit(uint64_t proposal_id) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) return false;
    if (it->second.deposit_returned) return false;
    // Mark as consumed (slashed = burned; actual transfer handled by caller)
    it->second.deposit_returned = true;
    return true;
}

// TreasuryManager Implementation
TreasuryManager::TreasuryManager() : balance_(0) {}

TreasuryManager::~TreasuryManager() = default;

bool TreasuryManager::Deposit(uint64_t amount, const std::vector<uint8_t>& from) {
    if (amount == 0) {
        return false;
    }

    balance_ += amount;

    TreasuryTransaction tx;
    tx.amount = amount;
    tx.address = from;
    tx.is_deposit = true;
    tx.timestamp = 0;  // Would use actual timestamp
    tx.proposal_id = 0;

    transactions_.push_back(tx);

    return true;
}

bool TreasuryManager::Withdraw(uint64_t amount, const std::vector<uint8_t>& to,
                               uint64_t proposal_id) {
    if (amount == 0 || amount > balance_) {
        return false;
    }

    if (proposal_id == 0) {
        return false;  // Requires approved proposal
    }

    balance_ -= amount;

    TreasuryTransaction tx;
    tx.amount = amount;
    tx.address = to;
    tx.is_deposit = false;
    tx.timestamp = 0;  // Would use actual timestamp
    tx.proposal_id = proposal_id;

    transactions_.push_back(tx);

    return true;
}

std::vector<TreasuryManager::TreasuryTransaction> TreasuryManager::GetTransactions() const {
    return transactions_;
}

// DelegationSystem Implementation
bool DelegationSystem::Delegate(const std::vector<uint8_t>& delegator,
                                const std::vector<uint8_t>& delegatee, uint64_t amount) {
    if (amount == 0 || delegator == delegatee) {
        return false;
    }

    delegations_[delegator][delegatee] += amount;

    return true;
}

bool DelegationSystem::Undelegate(const std::vector<uint8_t>& delegator,
                                  const std::vector<uint8_t>& delegatee, uint64_t amount) {
    auto it = delegations_.find(delegator);
    if (it == delegations_.end()) {
        return false;
    }

    auto& delegations = it->second;
    auto delegate_it = delegations.find(delegatee);
    if (delegate_it == delegations.end()) {
        return false;
    }

    if (delegate_it->second < amount) {
        return false;
    }

    delegate_it->second -= amount;

    if (delegate_it->second == 0) {
        delegations.erase(delegate_it);
    }

    return true;
}

uint64_t DelegationSystem::GetVotingPower(const std::vector<uint8_t>& address) const {
    uint64_t power = 0;

    // Add delegations to this address
    for (const auto& [delegator, delegations] : delegations_) {
        auto it = delegations.find(address);
        if (it != delegations.end()) {
            power += it->second;
        }
    }

    return power;
}

std::map<std::vector<uint8_t>, uint64_t>
DelegationSystem::GetDelegationsFrom(const std::vector<uint8_t>& delegator) const {
    auto it = delegations_.find(delegator);
    if (it == delegations_.end()) {
        return {};
    }
    return it->second;
}

std::map<std::vector<uint8_t>, uint64_t>
DelegationSystem::GetDelegationsTo(const std::vector<uint8_t>& delegatee) const {
    std::map<std::vector<uint8_t>, uint64_t> result;

    for (const auto& [delegator, delegations] : delegations_) {
        auto it = delegations.find(delegatee);
        if (it != delegations.end()) {
            result[delegator] = it->second;
        }
    }

    return result;
}

}  // namespace governance
}  // namespace parthenon
