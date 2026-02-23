#ifndef PARTHENON_GOVERNANCE_VOTING_H
#define PARTHENON_GOVERNANCE_VOTING_H

#include "antiwhale.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

// Forward declare so VotingSystem can optionally reference Boule without
// creating a circular dependency between voting.h and boule.h.
class Boule;

/**
 * Proposal Type
 */
enum class ProposalType {
    PARAMETER_CHANGE,   // Change blockchain parameter
    TREASURY_SPENDING,  // Spend from treasury
    PROTOCOL_UPGRADE,   // Upgrade protocol
    GENERAL,            // General governance decision
    CONSTITUTIONAL,     // Requires supermajority; cannot be emergency-tracked
    EMERGENCY           // Fast-tracked by Prytany; shorter voting period
};

/**
 * Proposal Status
 */
enum class ProposalStatus {
    PENDING,   // Awaiting votes
    ACTIVE,    // Currently being voted on
    PASSED,    // Proposal passed
    REJECTED,  // Proposal rejected
    EXECUTED,  // Proposal executed
    EXPIRED    // Voting period expired
};

/**
 * Vote Choice
 *
 * VETO – "strongly against; reject regardless of YES/NO ratio and slash the
 *         proposer's deposit."  If the veto share of all non-ABSTAIN votes
 *         exceeds veto_threshold_bps (default 3334 bps ≈ 33.34 %), the
 *         proposal is immediately REJECTED and the deposit is slashed.
 *         Modelled on the Cosmos Hub veto mechanism.
 */
enum class VoteChoice { YES, NO, ABSTAIN, VETO };

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
    uint64_t veto_votes;   // VETO ballots; if share > veto_threshold → reject + slash

    // Requirements
    uint64_t quorum_requirement;
    uint64_t approval_threshold;  // Percentage (0-100)
    uint64_t veto_threshold_bps;  // basis points; if veto share > this → auto-reject + slash deposit
                                  // default: 3334 (≈ 33.34 % – Cosmos Hub model)

    // Proposal deposit (Isegoria – anti-spam)
    uint64_t deposit_amount;   // tokens locked by proposer
    bool     deposit_returned; // true once deposit has been returned or slashed

    // Boule screening
    bool boule_approved;       // true once the Boule has approved this proposal

    Proposal()
        : proposal_id(0),
          type(ProposalType::GENERAL),
          status(ProposalStatus::PENDING),
          creation_time(0),
          voting_start(0),
          voting_end(0),
          execution_time(0),
          yes_votes(0),
          no_votes(0),
          abstain_votes(0),
          veto_votes(0),
          quorum_requirement(0),
          approval_threshold(50),
          veto_threshold_bps(3334),
          deposit_amount(0),
          deposit_returned(false),
          boule_approved(false) {}
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

    Vote() : proposal_id(0), choice(VoteChoice::ABSTAIN), voting_power(0), timestamp(0) {}
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
     * deposit_amount – tokens the proposer locks as anti-spam collateral.
     *                  Pass 0 when deposits are not enforced.
     */
    uint64_t CreateProposal(const std::vector<uint8_t>& proposer, ProposalType type,
                            const std::string& title, const std::string& description,
                            const std::vector<uint8_t>& execution_data,
                            uint64_t deposit_amount = 0);

    /**
     * Get proposal by ID
     */
    std::optional<Proposal> GetProposal(uint64_t proposal_id) const;

    /**
     * Cast vote
     */
    bool CastVote(uint64_t proposal_id, const std::vector<uint8_t>& voter, VoteChoice choice,
                  uint64_t voting_power, const std::vector<uint8_t>& signature);

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
     * Update the current block height (called by consensus layer on each new block)
     */
    void UpdateBlockHeight(uint64_t height) { current_block_height_ = height; }

    /**
     * Get current block height
     */
    uint64_t GetBlockHeight() const { return current_block_height_; }

    /**
     * Mark a proposal as Boule-approved (called by Boule integration layer).
     * Returns false if proposal not found.
     */
    bool MarkBouleApproved(uint64_t proposal_id);

    /**
     * Proposal deposit management.
     * ReturnDeposit – called after PASSED+EXECUTED or when proposal is not rejected.
     * SlashDeposit  – called when governance decides to punish the proposer (spam/rejection).
     */
    bool ReturnDeposit(uint64_t proposal_id);
    bool SlashDeposit(uint64_t proposal_id);

    /**
     * Set total token supply so the anti-whale guard can compute percentages.
     */
    void SetTotalSupply(uint64_t supply) { total_supply_ = supply; }
    uint64_t GetTotalSupply() const { return total_supply_; }

    /**
     * Attach an AntiWhaleGuard. Ownership stays with the caller.
     * Pass nullptr to detach (voting power passes through unmodified).
     */
    void SetAntiWhaleGuard(AntiWhaleGuard* guard) { anti_whale_ = guard; }

    /**
     * Attach a Boule instance for proposal screening integration.
     * Pass nullptr to detach.
     */
    void SetBoule(Boule* boule) { boule_ = boule; }

    /**
     * When enabled, CastVote is rejected unless the proposal's
     * boule_approved flag is set (or a Boule is attached and reports approval).
     */
    void SetRequireBouleApproval(bool required) { require_boule_approval_ = required; }
    bool GetRequireBouleApproval() const { return require_boule_approval_; }

    /**
     * Set voting parameters
     */
    void SetVotingPeriod(uint64_t blocks) { voting_period_ = blocks; }
    void SetDefaultQuorum(uint64_t amount) { default_quorum_ = amount; }
    void SetDefaultThreshold(uint64_t percent) { default_threshold_ = percent; }
    /** Veto threshold in basis points (default 3334 ≈ 33.34 %) */
    void SetVetoThreshold(uint64_t bps) { veto_threshold_bps_ = bps; }

    /**
     * Get voting parameters
     */
    uint64_t GetVotingPeriod() const { return voting_period_; }
    uint64_t GetDefaultQuorum() const { return default_quorum_; }
    uint64_t GetDefaultThreshold() const { return default_threshold_; }
    uint64_t GetVetoThreshold() const { return veto_threshold_bps_; }

  private:
    uint64_t next_proposal_id_;
    uint64_t current_block_height_;
    uint64_t voting_period_;
    uint64_t default_quorum_;
    uint64_t default_threshold_;
    uint64_t total_supply_;
    uint64_t veto_threshold_bps_;  // system-wide default veto threshold

    AntiWhaleGuard* anti_whale_;    // optional, not owned
    Boule*          boule_;         // optional, not owned
    bool            require_boule_approval_;

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
    bool Withdraw(uint64_t amount, const std::vector<uint8_t>& to, uint64_t proposal_id);

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
    bool Delegate(const std::vector<uint8_t>& delegator, const std::vector<uint8_t>& delegatee,
                  uint64_t amount);

    /**
     * Undelegate voting power
     */
    bool Undelegate(const std::vector<uint8_t>& delegator, const std::vector<uint8_t>& delegatee,
                    uint64_t amount);

    /**
     * Get total voting power for address (including delegations)
     */
    uint64_t GetVotingPower(const std::vector<uint8_t>& address) const;

    /**
     * Get delegations from address
     */
    std::map<std::vector<uint8_t>, uint64_t>
    GetDelegationsFrom(const std::vector<uint8_t>& delegator) const;

    /**
     * Get delegations to address
     */
    std::map<std::vector<uint8_t>, uint64_t>
    GetDelegationsTo(const std::vector<uint8_t>& delegatee) const;

  private:
    // Map: delegator -> (delegatee -> amount)
    std::map<std::vector<uint8_t>, std::map<std::vector<uint8_t>, uint64_t>> delegations_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_VOTING_H
