#ifndef PARTHENON_GOVERNANCE_EVENTLOG_H
#define PARTHENON_GOVERNANCE_EVENTLOG_H

#include <cstdint>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * GovernanceEventLog
 *
 * Unified, append-only audit trail for every governance event across all
 * subsystems.  All modules (VotingSystem, Treasury, StakingRegistry,
 * Boule, Ostracism, EmergencyCouncil) append events here so operators
 * have a single queryable log.
 *
 * Ancient-Greece analogy
 * ----------------------
 *  Stele  – Athenians inscribed laws and decrees on stone stelae in the
 *            Agora for public inspection.  The EventLog is the on-chain
 *            equivalent: immutable, public, and permanently accessible.
 *
 * All append operations are O(1); query operations are O(n) by design
 * (the log is primarily for off-chain indexers and auditors).
 */
class GovernanceEventLog {
  public:
    // ------------------------------------------------------------------ //
    //  Event types                                                         //
    // ------------------------------------------------------------------ //

    enum class EventType : uint32_t {
        // Proposal lifecycle
        PROPOSAL_CREATED        = 100,
        PROPOSAL_BOULE_APPROVED = 101,
        PROPOSAL_BOULE_REJECTED = 102,
        PROPOSAL_VOTE_CAST      = 103,
        PROPOSAL_PASSED         = 104,
        PROPOSAL_REJECTED       = 105,
        PROPOSAL_EXECUTED       = 106,
        PROPOSAL_EXPIRED        = 107,
        PROPOSAL_CANCELLED      = 108,  // by EmergencyCouncil

        // Deposit management
        PROPOSAL_DEPOSIT_PAID   = 110,
        PROPOSAL_DEPOSIT_RETURNED = 111,
        PROPOSAL_DEPOSIT_SLASHED  = 112,

        // Treasury
        TREASURY_DEPOSIT            = 200,
        TREASURY_SPEND              = 201,
        TREASURY_GRANT_CREATED      = 202,
        TREASURY_MILESTONE_RELEASED = 203,
        TREASURY_GRANT_REVOKED      = 204,
        TREASURY_MULTISIG_PROPOSED  = 205,
        TREASURY_MULTISIG_SIGNED    = 206,
        TREASURY_MULTISIG_EXECUTED  = 207,
        TREASURY_BUDGET_CREATED     = 208,

        // Staking
        STAKE_DEPOSITED           = 300,
        STAKE_UNSTAKE_REQUESTED   = 301,
        STAKE_UNSTAKE_CLAIMED     = 302,
        STAKE_SLASHED             = 303,

        // Boule
        BOULE_CITIZEN_REGISTERED  = 400,
        BOULE_SORTITION_CONDUCTED = 401,
        BOULE_PROPOSAL_REVIEWED   = 402,
        BOULE_CHALLENGE_RAISED    = 403,
        BOULE_CHALLENGE_RESOLVED  = 404,

        // Ostracism
        OSTRACISM_NOMINATED       = 500,
        OSTRACISM_VOTE_CAST       = 501,
        OSTRACISM_ENACTED         = 502,
        OSTRACISM_REHABILITATED   = 503,

        // Emergency council
        EMERGENCY_GUARDIAN_ADDED    = 600,
        EMERGENCY_GUARDIAN_REMOVED  = 601,
        EMERGENCY_ACTION_PROPOSED   = 602,
        EMERGENCY_ACTION_SIGNED     = 603,
        EMERGENCY_ACTION_EXECUTED   = 604,

        // Governance parameters
        PARAM_CHANGED               = 700,
    };

    // ------------------------------------------------------------------ //
    //  Event record                                                        //
    // ------------------------------------------------------------------ //

    struct Event {
        uint64_t             event_id;
        EventType            type;
        uint64_t             block_height;
        std::vector<uint8_t> actor;         // address that triggered event
        uint64_t             reference_id;  // proposal_id, grant_id, etc.
        std::string          description;
    };

    // ------------------------------------------------------------------ //
    //  Append                                                              //
    // ------------------------------------------------------------------ //

    void Log(EventType type, uint64_t block_height,
             const std::vector<uint8_t>& actor,
             uint64_t reference_id,
             const std::string& description);

    // ------------------------------------------------------------------ //
    //  Query                                                               //
    // ------------------------------------------------------------------ //

    const std::vector<Event>& GetAll() const { return events_; }

    std::vector<Event> GetByType(EventType type) const;

    std::vector<Event> GetByActor(const std::vector<uint8_t>& actor) const;

    std::vector<Event> GetByBlockRange(uint64_t from_block,
                                       uint64_t to_block) const;

    std::vector<Event> GetByReferenceId(uint64_t reference_id) const;

    size_t Size() const { return events_.size(); }

  private:
    std::vector<Event> events_;
    uint64_t           next_event_id_ = 1;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_EVENTLOG_H
