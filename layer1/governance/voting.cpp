#include "voting.h"
#include <algorithm>

namespace parthenon {
namespace governance {

// VotingSystem Implementation
VotingSystem::VotingSystem()
    : next_proposal_id_(1)
    , current_block_height_(0)
    , voting_period_(10000)      // Default: 10000 blocks
    , default_quorum_(1000000)   // Default quorum
    , default_threshold_(50)     // 50% approval
{
}

VotingSystem::~VotingSystem() = default;

uint64_t VotingSystem::CreateProposal(
    const std::vector<uint8_t>& proposer,
    ProposalType type,
    const std::string& title,
    const std::string& description,
    const std::vector<uint8_t>& execution_data)
{
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
    proposal.approval_threshold = default_threshold_;
    proposal.execution_data = execution_data;
    
    proposals_[proposal.proposal_id] = proposal;
    
    return proposal.proposal_id;
}

std::optional<Proposal> VotingSystem::GetProposal(uint64_t proposal_id) const
{
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool VotingSystem::CastVote(
    uint64_t proposal_id,
    const std::vector<uint8_t>& voter,
    VoteChoice choice,
    uint64_t voting_power,
    const std::vector<uint8_t>& signature)
{
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }
    
    Proposal& proposal = it->second;
    
    // Check voting period
    if (current_block_height_ < proposal.voting_start ||
        current_block_height_ > proposal.voting_end) {
        return false;
    }
    
    // Check if already voted
    if (HasVoted(proposal_id, voter)) {
        return false;
    }
    
    // Verify signature (simplified)
    if (signature.empty()) {
        return false;
    }
    
    // Create vote record
    Vote vote;
    vote.proposal_id = proposal_id;
    vote.voter = voter;
    vote.choice = choice;
    vote.voting_power = voting_power;
    vote.timestamp = current_block_height_;
    vote.signature = signature;
    
    // Add to votes
    votes_[proposal_id].push_back(vote);
    
    // Update tallies
    switch (choice) {
        case VoteChoice::YES:
            proposal.yes_votes += voting_power;
            break;
        case VoteChoice::NO:
            proposal.no_votes += voting_power;
            break;
        case VoteChoice::ABSTAIN:
            proposal.abstain_votes += voting_power;
            break;
    }
    
    // Update status
    if (proposal.status == ProposalStatus::PENDING) {
        proposal.status = ProposalStatus::ACTIVE;
    }
    
    return true;
}

bool VotingSystem::TallyVotes(uint64_t proposal_id)
{
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }
    
    Proposal& proposal = it->second;
    
    // Check voting period has ended
    if (current_block_height_ <= proposal.voting_end) {
        return false;
    }
    
    // Calculate total votes
    uint64_t total_votes = proposal.yes_votes + proposal.no_votes + proposal.abstain_votes;
    
    // Check quorum
    if (total_votes < proposal.quorum_requirement) {
        proposal.status = ProposalStatus::REJECTED;
        return true;
    }
    
    // Calculate approval percentage
    uint64_t approval_votes = proposal.yes_votes + proposal.no_votes;
    if (approval_votes == 0) {
        proposal.status = ProposalStatus::REJECTED;
        return true;
    }
    
    uint64_t approval_percent = (proposal.yes_votes * 100) / approval_votes;
    
    // Check if passed
    if (approval_percent >= proposal.approval_threshold) {
        proposal.status = ProposalStatus::PASSED;
        proposal.execution_time = current_block_height_ + 1000;  // Execute after 1000 blocks
    } else {
        proposal.status = ProposalStatus::REJECTED;
    }
    
    return true;
}

bool VotingSystem::ExecuteProposal(uint64_t proposal_id)
{
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
    
    // Execute proposal based on type
    // In production, this would trigger actual state changes
    switch (proposal.type) {
        case ProposalType::PARAMETER_CHANGE:
            // Apply parameter change
            break;
        case ProposalType::TREASURY_SPENDING:
            // Execute treasury spending
            break;
        case ProposalType::PROTOCOL_UPGRADE:
            // Schedule protocol upgrade
            break;
        case ProposalType::GENERAL:
            // General action
            break;
    }
    
    proposal.status = ProposalStatus::EXECUTED;
    
    return true;
}

std::vector<Proposal> VotingSystem::GetActiveProposals() const
{
    std::vector<Proposal> active;
    
    for (const auto& [id, proposal] : proposals_) {
        if (proposal.status == ProposalStatus::ACTIVE ||
            proposal.status == ProposalStatus::PENDING) {
            active.push_back(proposal);
        }
    }
    
    return active;
}

std::vector<Vote> VotingSystem::GetProposalVotes(uint64_t proposal_id) const
{
    auto it = votes_.find(proposal_id);
    if (it == votes_.end()) {
        return {};
    }
    return it->second;
}

bool VotingSystem::HasVoted(
    uint64_t proposal_id,
    const std::vector<uint8_t>& voter) const
{
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

// TreasuryManager Implementation
TreasuryManager::TreasuryManager()
    : balance_(0)
{
}

TreasuryManager::~TreasuryManager() = default;

bool TreasuryManager::Deposit(uint64_t amount, const std::vector<uint8_t>& from)
{
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

bool TreasuryManager::Withdraw(
    uint64_t amount,
    const std::vector<uint8_t>& to,
    uint64_t proposal_id)
{
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

std::vector<TreasuryManager::TreasuryTransaction> TreasuryManager::GetTransactions() const
{
    return transactions_;
}

// DelegationSystem Implementation
bool DelegationSystem::Delegate(
    const std::vector<uint8_t>& delegator,
    const std::vector<uint8_t>& delegatee,
    uint64_t amount)
{
    if (amount == 0 || delegator == delegatee) {
        return false;
    }
    
    delegations_[delegator][delegatee] += amount;
    
    return true;
}

bool DelegationSystem::Undelegate(
    const std::vector<uint8_t>& delegator,
    const std::vector<uint8_t>& delegatee,
    uint64_t amount)
{
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

uint64_t DelegationSystem::GetVotingPower(const std::vector<uint8_t>& address) const
{
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

std::map<std::vector<uint8_t>, uint64_t> DelegationSystem::GetDelegationsFrom(
    const std::vector<uint8_t>& delegator) const
{
    auto it = delegations_.find(delegator);
    if (it == delegations_.end()) {
        return {};
    }
    return it->second;
}

std::map<std::vector<uint8_t>, uint64_t> DelegationSystem::GetDelegationsTo(
    const std::vector<uint8_t>& delegatee) const
{
    std::map<std::vector<uint8_t>, uint64_t> result;
    
    for (const auto& [delegator, delegations] : delegations_) {
        auto it = delegations.find(delegatee);
        if (it != delegations.end()) {
            result[delegator] = it->second;
        }
    }
    
    return result;
}

} // namespace governance
} // namespace parthenon
