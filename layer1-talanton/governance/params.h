#ifndef PARTHENON_GOVERNANCE_PARAMS_H
#define PARTHENON_GOVERNANCE_PARAMS_H

#include <cstdint>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * GovernanceParams
 *
 * Single source of truth for all on-chain governance configuration.
 * Parameters can only be changed through an executed governance proposal
 * (i.e. UpdateParam requires a non-zero proposal_id).
 *
 * Constitutional floors / ceilings are enforced: no proposal can push a
 * parameter outside its hard limits, ensuring the system cannot vote
 * itself into an insecure or unusable state (Isonomia principle).
 *
 * Ancient Greece mapping
 * ----------------------
 *  Isonomia   – equality before the law → constitutional min/max guards.
 *  Isegoria   – equal right to speak    → min_proposal_deposit keeps
 *                access open without allowing spam.
 *  Graphe     – unconstitutionality     → constitutional_threshold_bps
 *               requires supermajority for CONSTITUTIONAL proposals.
 */
class GovernanceParams {
  public:
    struct Params {
        // ---- Voting timing -------------------------------------------------
        uint64_t voting_delay_blocks;        // Delay after creation before voting opens
        uint64_t voting_period_blocks;       // Duration of the voting window
        uint64_t execution_delay_blocks;     // Time-lock after passing, before execution
        uint64_t max_proposal_age_blocks;    // Proposal expires if not executed by this age

        // ---- Thresholds ----------------------------------------------------
        uint64_t default_quorum;             // Minimum total weighted votes
        uint64_t default_threshold_bps;      // Approval % in basis points (5000 = 50 %)
        uint64_t constitutional_threshold_bps; // For CONSTITUTIONAL proposals (e.g. 6667)

        // ---- Proposal deposit (Isegoria) -----------------------------------
        uint64_t min_proposal_deposit;       // Required deposit to submit proposal
        bool     slash_deposit_on_rejection; // Burn deposit when proposal is rejected
        bool     slash_deposit_on_spam;      // Burn deposit when proposal does not meet quorum

        // ---- Anti-whale (Athenian equality principle) ----------------------
        bool     quadratic_voting_enabled;
        uint64_t max_voting_power_cap;       // Hard cap per voter; 0 = disabled
        uint64_t whale_threshold_bps;        // Basis points of supply = whale

        // ---- Boule (council) -----------------------------------------------
        uint32_t boule_size;                 // Number of council members
        uint64_t boule_term_blocks;          // Council term length
        uint64_t boule_min_stake;            // Dokimasia: min stake to register
        bool     boule_screening_required;   // Must pass Boule before assembly vote

        // ---- Voting thresholds (including VETO) ----------------------------
        uint64_t veto_threshold_bps;         // If veto share > this → auto-reject + slash
                                             // Default: 3334 bps (≈ 33.34 % – Cosmos Hub)

        // ---- Ostracism (Athenian safety valve) -----------------------------
        uint64_t ostracism_ban_duration_blocks;
        uint64_t ostracism_required_votes;
    };

    // ------------------------------------------------------------------ //
    //  Constitutional limits (hard-coded, cannot be changed by proposal)  //
    // ------------------------------------------------------------------ //

    struct Limits {
        uint64_t min_voting_period_blocks;     // Floor: 100 blocks
        uint64_t max_voting_period_blocks;     // Ceiling: 10 * 50400 blocks
        uint64_t min_constitutional_threshold; // Floor: 5001 bps (> 50 %)
        uint64_t min_default_threshold;        // Floor: 3334 bps (> 1/3)
        uint64_t max_boule_size;               // Ceiling: 500 (Athens had 500)
        uint64_t min_veto_threshold;           // Floor: 1000 bps (10 %) – can't be too easy to veto
        uint64_t max_veto_threshold;           // Ceiling: 5000 bps (50 %) – veto must be reachable
    };

    static constexpr Limits kLimits{
        /*min_voting_period_blocks     =*/100,
        /*max_voting_period_blocks     =*/504000,
        /*min_constitutional_threshold =*/5001,
        /*min_default_threshold        =*/3334,
        /*max_boule_size               =*/500,
        /*min_veto_threshold           =*/1000,
        /*max_veto_threshold           =*/5000,
    };

    // Default sensible parameters at genesis
    static Params Defaults();

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    explicit GovernanceParams(const Params& initial = Defaults());

    // ------------------------------------------------------------------ //
    //  Read                                                                //
    // ------------------------------------------------------------------ //

    const Params& Get() const { return params_; }

    // ------------------------------------------------------------------ //
    //  Write (proposal-gated)                                             //
    // ------------------------------------------------------------------ //

    /**
     * Update a named uint64_t parameter.  Requires proposal_id != 0.
     * Returns false when the key is unknown or the value violates limits.
     */
    bool UpdateParam(const std::string& key, uint64_t value,
                     uint64_t proposal_id, uint64_t block_height);

    /**
     * Update a named bool parameter.  Requires proposal_id != 0.
     */
    bool UpdateBoolParam(const std::string& key, bool value,
                         uint64_t proposal_id, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Change history (immutable audit trail)                             //
    // ------------------------------------------------------------------ //

    struct ParamChange {
        std::string key;
        uint64_t    old_value;
        uint64_t    new_value;
        uint64_t    proposal_id;
        uint64_t    changed_at_block;
    };

    const std::vector<ParamChange>& GetChangeHistory() const { return history_; }

  private:
    Params                  params_;
    std::vector<ParamChange> history_;

    // Returns false when the proposed (key, value) pair violates constitutional limits.
    bool ValidateUint(const std::string& key, uint64_t value) const;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_PARAMS_H
