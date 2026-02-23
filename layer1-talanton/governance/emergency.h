#ifndef PARTHENON_GOVERNANCE_EMERGENCY_H
#define PARTHENON_GOVERNANCE_EMERGENCY_H

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * EmergencyCouncil
 *
 * A small, trusted M-of-N multi-signature council empowered to act
 * faster than the normal governance cycle in critical situations
 * (security exploits, chain halts, malicious proposals).
 *
 * Ancient-Greece analogy
 * ----------------------
 *  Apophasis  – An Athenian board of special investigators with power
 *               to act on imminent threats to the democracy.  Here the
 *               EmergencyCouncil fulfils that role, but its power is
 *               intentionally narrow and time-limited.
 *
 * Action types
 * ------------
 *  PAUSE_GOVERNANCE   – halt new proposal creation / voting temporarily.
 *  CANCEL_PROPOSAL    – veto a specific on-chain proposal (e.g. malicious).
 *  FAST_TRACK_UPGRADE – promote an EMERGENCY-type proposal to immediate
 *                       execution after reduced voting window.
 *  CUSTOM             – arbitrary memo-only action (no on-chain effect
 *                       beyond the record; off-chain procedures govern it).
 *
 * Security properties
 * -------------------
 *  • Guardians are added/removed by governance (not self-appointed).
 *  • Every action requires >= required_sigs_ distinct guardian signatures.
 *  • Actions expire after action_ttl_blocks if not fully signed.
 *  • No guardian can sign the same action twice.
 *  • Actions are immutable once executed.
 */
class EmergencyCouncil {
  public:
    // ------------------------------------------------------------------ //
    //  Data types                                                          //
    // ------------------------------------------------------------------ //

    enum class ActionType {
        PAUSE_GOVERNANCE,
        CANCEL_PROPOSAL,
        FAST_TRACK_UPGRADE,
        CUSTOM
    };

    struct Action {
        uint64_t                        action_id;
        ActionType                      type;
        std::string                     description;
        std::vector<uint8_t>            initiator;
        uint64_t                        target_proposal_id;  // for CANCEL / FAST_TRACK
        uint64_t                        proposed_at_block;
        uint64_t                        expires_at_block;
        bool                            executed;
        uint64_t                        executed_at_block;
        std::set<std::vector<uint8_t>>  signers;
    };

    struct Guardian {
        std::vector<uint8_t> address;
        std::string          role;   // "security", "core-dev", "community", …
        uint64_t             added_at_block;
    };

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    /**
     * required_sigs      – M in M-of-N (min guardian signatures to execute).
     * action_ttl_blocks  – actions that have not gathered enough signatures
     *                      within this window are considered expired.
     */
    explicit EmergencyCouncil(uint32_t required_sigs    = 3,
                              uint64_t action_ttl_blocks = 1200);  // ~4 hours

    // ------------------------------------------------------------------ //
    //  Guardian management (done by governance, not self)                 //
    // ------------------------------------------------------------------ //

    bool AddGuardian(const std::vector<uint8_t>& address,
                     const std::string& role, uint64_t block_height);
    bool RemoveGuardian(const std::vector<uint8_t>& address);
    bool IsGuardian(const std::vector<uint8_t>& address) const;
    std::vector<Guardian> GetGuardians() const;

    // ------------------------------------------------------------------ //
    //  Action lifecycle                                                    //
    // ------------------------------------------------------------------ //

    /**
     * Propose a new emergency action.  The initiator must be a guardian
     * and counts as the first signature.
     * Returns action_id (>0) on success, 0 on failure.
     */
    uint64_t ProposeAction(ActionType type, const std::string& description,
                           const std::vector<uint8_t>& initiator,
                           uint64_t target_proposal_id,
                           uint64_t block_height);

    /**
     * Add a signature from another guardian.
     */
    bool SignAction(uint64_t action_id, const std::vector<uint8_t>& guardian,
                   uint64_t block_height);

    /**
     * Execute the action once sufficient signatures are collected and
     * the action has not expired.
     */
    bool ExecuteAction(uint64_t action_id, uint64_t block_height);

    bool HasSufficientSignatures(uint64_t action_id) const;
    bool IsExpired(uint64_t action_id, uint64_t block_height) const;

    std::optional<Action> GetAction(uint64_t action_id) const;

    // ------------------------------------------------------------------ //
    //  Governance-pause state query                                        //
    // ------------------------------------------------------------------ //

    /**
     * Returns true if a PAUSE_GOVERNANCE action has been executed and
     * no subsequent un-pause action has been executed.
     */
    bool IsGovernancePaused() const { return governance_paused_; }

    // ------------------------------------------------------------------ //
    //  Configuration                                                       //
    // ------------------------------------------------------------------ //

    void     SetRequiredSigs(uint32_t n)    { required_sigs_ = n; }
    uint32_t GetRequiredSigs() const        { return required_sigs_; }
    void     SetActionTTL(uint64_t blocks)  { action_ttl_ = blocks; }
    uint64_t GetActionTTL() const           { return action_ttl_; }

  private:
    uint32_t required_sigs_;
    uint64_t action_ttl_;
    bool     governance_paused_ = false;

    std::vector<Guardian>         guardians_;
    std::map<uint64_t, Action>    actions_;
    uint64_t                      next_action_id_ = 1;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_EMERGENCY_H
