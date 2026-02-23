#ifndef PARTHENON_GOVERNANCE_VESTING_H
#define PARTHENON_GOVERNANCE_VESTING_H

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * VestingRegistry
 *
 * Cliff + linear vesting schedules for treasury grants and team allocations.
 *
 * Ancient-Greece analogy
 * ----------------------
 *  Misthophoria – the Athenian system of paying citizens for public service
 *  on a time-deferred basis.  Vesting ensures long-term commitment: tokens
 *  are allocated upfront but only claimable over time.
 *
 * Schedule lifecycle
 * ------------------
 *  CREATED   → beneficiary can call Claimable() / Claim()
 *  REVOKED   → remaining unvested tokens returned to treasury (governance)
 *
 * Vesting formula (linear after cliff)
 * -------------------------------------
 *  vested(block) =
 *    0                                         if block < start + cliff
 *    total                                     if block >= start + cliff + duration
 *    total * (block - (start + cliff))
 *          / duration                          otherwise
 *
 * Claimable(block) = vested(block) − already_claimed
 *
 * Notes
 * -----
 *  • A grant_id of 0 means the schedule is standalone (not linked to a
 *    Treasury grant).
 *  • The actual token transfer is the caller's responsibility; Claim()
 *    only advances the accounting.
 */
class VestingRegistry {
  public:
    struct VestingSchedule {
        uint64_t             schedule_id;
        std::vector<uint8_t> beneficiary;
        uint64_t             total_amount;      // tokens to vest in full
        uint64_t             start_block;
        uint64_t             cliff_blocks;      // no release before start + cliff
        uint64_t             duration_blocks;   // linear release period after cliff
        uint64_t             claimed_amount;    // cumulative amount already claimed
        bool                 revoked;
        uint64_t             revoked_at_block;
        uint64_t             grant_id;          // 0 = standalone
    };

    // ------------------------------------------------------------------ //
    //  Creating schedules                                                  //
    // ------------------------------------------------------------------ //

    /**
     * Create a new vesting schedule.
     * Returns schedule_id > 0 on success; 0 if parameters are invalid.
     *
     * Conditions for invalid:
     *  - beneficiary is empty
     *  - total_amount == 0
     *  - duration_blocks == 0
     */
    uint64_t CreateSchedule(const std::vector<uint8_t>& beneficiary,
                            uint64_t total_amount,
                            uint64_t start_block,
                            uint64_t cliff_blocks,
                            uint64_t duration_blocks,
                            uint64_t grant_id = 0);

    // ------------------------------------------------------------------ //
    //  Vesting arithmetic                                                  //
    // ------------------------------------------------------------------ //

    /**
     * Total vested amount (earned, regardless of claimed status).
     */
    uint64_t GetTotalVested(uint64_t schedule_id, uint64_t current_block) const;

    /**
     * Amount available to claim right now (vested − claimed).
     */
    uint64_t GetClaimable(uint64_t schedule_id, uint64_t current_block) const;

    // ------------------------------------------------------------------ //
    //  Claiming                                                            //
    // ------------------------------------------------------------------ //

    /**
     * Claim all currently claimable tokens.
     * Returns the amount claimed (0 if nothing is claimable or schedule
     * is revoked/not found).
     */
    uint64_t Claim(uint64_t schedule_id, uint64_t current_block);

    // ------------------------------------------------------------------ //
    //  Revocation (governance-gated)                                      //
    // ------------------------------------------------------------------ //

    /**
     * Revoke a schedule.  After revocation no more tokens can be claimed.
     * Returns the unvested amount that should be returned to the treasury.
     * Requires proposal_id != 0 (same governance-gate as Treasury spends).
     */
    uint64_t Revoke(uint64_t schedule_id, uint64_t proposal_id,
                    uint64_t current_block);

    /**
     * Amount that WOULD be returned to treasury if revoked right now.
     * (total_amount - vested_at_current_block)
     */
    uint64_t GetReclaimable(uint64_t schedule_id, uint64_t current_block) const;

    // ------------------------------------------------------------------ //
    //  Queries                                                             //
    // ------------------------------------------------------------------ //

    std::optional<VestingSchedule> GetSchedule(uint64_t schedule_id) const;

    std::vector<VestingSchedule>
    GetSchedulesForBeneficiary(const std::vector<uint8_t>& beneficiary) const;

    size_t Count() const { return schedules_.size(); }

  private:
    std::map<uint64_t, VestingSchedule> schedules_;
    uint64_t next_id_ = 1;

    uint64_t ComputeVested(const VestingSchedule& s, uint64_t current_block) const;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_VESTING_H
