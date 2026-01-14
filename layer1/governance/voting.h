#ifndef PARTHENON_GOVERNANCE_VOTING_H
#define PARTHENON_GOVERNANCE_VOTING_H

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <optional>

namespace parthenon {
namespace governance {

/**
 * Proposal Type
 */
enum class ProposalType {
    PARAMETER_CHANGE,    // Change blockchain parameter
    TREASURY_SPENDING,   // Spend from treasury
    PROTOCOL_UPGRADE,    // Upgrade protocol
    GENERAL              // General governance decision
};

/**
 * Proposal Status
 */
enum class ProposalStatus {
    PENDING,      // Awaiting votes
    ACTIVE,       // Currently being voted on
    PASSED,       // Proposal passed
    REJECTED,     // Proposal rejected
    EXECUTED,     // Proposal executed
    EXPIRED       // Voting period expired
};

/**
 * Vote Choice
 */
enum class VoteChoice {
    YES,
    NO,
    ABSTAIN
};

/**
 * Governance Proposal
 */
struct Proposal {
    uint64_t proposal_id;
    ProposalType type;
    ProposalStatus status;
    std::string title;
    std::string description;
    std::vector<uint8_t> proposer;
    uint64_t creation_time;
    uint64_t voting_start;
    uint64_t voting_end;
    uint64_t execution_time;
    std::vector<uint8_t> execution_data;
    
    // Vote tallies
    uint64_t yes_votes;
    uint64_t no_votes;
    uint64_t abstain_votes;
    
    // Requirements
    uint64_t quorum_requirement;
    uint64_t approval_threshold;  // Percentage (0-100)
    
    Proposal()
        : proposal_id(0)
        , type(ProposalType::GENERAL)
        , status(ProposalStatus::PENDING)
        , creation_time(0)
        , voting_start(0)
        , voting_end(0)
        , execution_time(0)
        , yes_votes(0)
        , no_votes(0)
        , abstain_votes(0)
        , quorum_requirement(0)
        , approval_threshold(50) {}
};

/**
 * Vote Record
 */
struct Vote {
    uint64_t proposal_id;
    std::vector<uint8_t> voter;
    VoteChoice choice;
    uint64_t voting_power;
    uint64_t timestamp;
    std::vector<uint8_t> signature;
    
    Vote()
        : proposal_id(0)
        , choice(VoteChoice::ABSTAIN)
        , voting_power(0)
        , timestamp(0) {}
};

/**
 * Voting System
 * Manages on-chain governance voting
 */
class VotingSystem {
public:
    VotingSystem();
    ~VotingSystem();
    
    /**
     * Create new proposal
     */
    uint64_t CreateProposal(
        const std::vector<uint8_t>& proposer,
        ProposalType type,
        const std::string& title,
        const std::string& description,
        const std::vector<uint8_t>& execution_data
    );
    
    /**
     * Get proposal by ID
     */
    std::optional<Proposal> GetProposal(uint64_t proposal_id) const;
    
    /**
     * Cast vote
     */
    bool CastVote(
        uint64_t proposal_id,
        const std::vector<uint8_t>& voter,
        VoteChoice choice,
        uint64_t voting_power,
        const std::vector<uint8_t>& signature
    );
    
    /**
     * Tally votes for proposal
     */
    bool TallyVotes(uint64_t proposal_id);
    
    /**
     * Execute passed proposal
     */
    bool ExecuteProposal(uint64_t proposal_id);
    
    /**
     * Get all active proposals
     */
    std::vector<Proposal> GetActiveProposals() const;
    
    /**
     * Get votes for proposal
     */
    std::vector<Vote> GetProposalVotes(uint64_t proposal_id) const;
    
    /**
     * Check if address has voted
     */
    bool HasVoted(uint64_t proposal_id, const std::vector<uint8_t>& voter) const;
    
    /**
     * Set voting parameters
     */
    void SetVotingPeriod(uint64_t blocks) { voting_period_ = blocks; }
    void SetDefaultQuorum(uint64_t amount) { default_quorum_ = amount; }
    void SetDefaultThreshold(uint64_t percent) { default_threshold_ = percent; }
    
    /**
     * Get voting parameters
     */
    uint64_t GetVotingPeriod() const { return voting_period_; }
    uint64_t GetDefaultQuorum() const { return default_quorum_; }
    uint64_t GetDefaultThreshold() const { return default_threshold_; }
    
private:
    uint64_t next_proposal_id_;
    uint64_t current_block_height_;
    uint64_t voting_period_;
    uint64_t default_quorum_;
    uint64_t default_threshold_;
    
    std::map<uint64_t, Proposal> proposals_;
    std::map<uint64_t, std::vector<Vote>> votes_;
};

/**
 * Treasury Management
 * Manages on-chain treasury funds
 */
class TreasuryManager {
public:
    TreasuryManager();
    ~TreasuryManager();
    
    /**
     * Deposit to treasury
     */
    bool Deposit(uint64_t amount, const std::vector<uint8_t>& from);
    
    /**
     * Withdraw from treasury (requires proposal)
     */
    bool Withdraw(
        uint64_t amount,
        const std::vector<uint8_t>& to,
        uint64_t proposal_id
    );
    
    /**
     * Get treasury balance
     */
    uint64_t GetBalance() const { return balance_; }
    
    /**
     * Get treasury transactions
     */
    struct TreasuryTransaction {
        uint64_t amount;
        std::vector<uint8_t> address;
        bool is_deposit;
        uint64_t timestamp;
        uint64_t proposal_id;  // 0 for deposits
    };
    
    std::vector<TreasuryTransaction> GetTransactions() const;
    
private:
    uint64_t balance_;
    std::vector<TreasuryTransaction> transactions_;
};

/**
 * Delegation System
 * Allows token holders to delegate voting power
 */
class DelegationSystem {
public:
    /**
     * Delegate voting power to another address
     */
    bool Delegate(
        const std::vector<uint8_t>& delegator,
        const std::vector<uint8_t>& delegatee,
        uint64_t amount
    );
    
    /**
     * Undelegate voting power
     */
    bool Undelegate(
        const std::vector<uint8_t>& delegator,
        const std::vector<uint8_t>& delegatee,
        uint64_t amount
    );
    
    /**
     * Get total voting power for address (including delegations)
     */
    uint64_t GetVotingPower(const std::vector<uint8_t>& address) const;
    
    /**
     * Get delegations from address
     */
    std::map<std::vector<uint8_t>, uint64_t> GetDelegationsFrom(
        const std::vector<uint8_t>& delegator
    ) const;
    
    /**
     * Get delegations to address
     */
    std::map<std::vector<uint8_t>, uint64_t> GetDelegationsTo(
        const std::vector<uint8_t>& delegatee
    ) const;
    
private:
    // Map: delegator -> (delegatee -> amount)
    std::map<std::vector<uint8_t>, std::map<std::vector<uint8_t>, uint64_t>> delegations_;
};

} // namespace governance
} // namespace parthenon

#endif // PARTHENON_GOVERNANCE_VOTING_H
