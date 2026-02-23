#include "vesting.h"

#include <algorithm>

namespace parthenon {
namespace governance {

// ---------------------------------------------------------------------------
// Private: core vesting arithmetic
// ---------------------------------------------------------------------------

uint64_t VestingRegistry::ComputeVested(const VestingSchedule& s,
                                         uint64_t current_block) const {
    if (s.total_amount == 0 || s.duration_blocks == 0) return 0;

    // Before cliff: nothing vested
    uint64_t cliff_end = s.start_block + s.cliff_blocks;
    if (current_block < cliff_end) return 0;

    // After full vesting period: fully vested
    uint64_t vest_end = cliff_end + s.duration_blocks;
    if (current_block >= vest_end) return s.total_amount;

    // Linear: total * elapsed / duration, split to avoid overflow.
    // total (up to ~1e15) * elapsed (up to ~500 000) can exceed uint64_t,
    // so we compute (total/duration)*elapsed + (total%duration)*elapsed/duration.
    uint64_t elapsed = current_block - cliff_end;
    return (s.total_amount / s.duration_blocks) * elapsed
         + (s.total_amount % s.duration_blocks) * elapsed / s.duration_blocks;
}

// ---------------------------------------------------------------------------
// Creating schedules
// ---------------------------------------------------------------------------

uint64_t VestingRegistry::CreateSchedule(const std::vector<uint8_t>& beneficiary,
                                          uint64_t total_amount,
                                          uint64_t start_block,
                                          uint64_t cliff_blocks,
                                          uint64_t duration_blocks,
                                          uint64_t grant_id) {
    if (beneficiary.empty() || total_amount == 0 || duration_blocks == 0) return 0;

    VestingSchedule s;
    s.schedule_id    = next_id_++;
    s.beneficiary    = beneficiary;
    s.total_amount   = total_amount;
    s.start_block    = start_block;
    s.cliff_blocks   = cliff_blocks;
    s.duration_blocks = duration_blocks;
    s.claimed_amount = 0;
    s.revoked        = false;
    s.revoked_at_block = 0;
    s.grant_id       = grant_id;

    schedules_[s.schedule_id] = s;
    return s.schedule_id;
}

// ---------------------------------------------------------------------------
// Vesting arithmetic
// ---------------------------------------------------------------------------

uint64_t VestingRegistry::GetTotalVested(uint64_t schedule_id,
                                          uint64_t current_block) const {
    auto it = schedules_.find(schedule_id);
    if (it == schedules_.end()) return 0;
    const VestingSchedule& s = it->second;
    if (s.revoked) {
        // When revoked, vested is frozen at the revocation block
        return ComputeVested(s, s.revoked_at_block);
    }
    return ComputeVested(s, current_block);
}

uint64_t VestingRegistry::GetClaimable(uint64_t schedule_id,
                                        uint64_t current_block) const {
    auto it = schedules_.find(schedule_id);
    if (it == schedules_.end()) return 0;
    const VestingSchedule& s = it->second;
    if (s.revoked) return 0;

    uint64_t vested = ComputeVested(s, current_block);
    return (vested > s.claimed_amount) ? vested - s.claimed_amount : 0;
}

// ---------------------------------------------------------------------------
// Claiming
// ---------------------------------------------------------------------------

uint64_t VestingRegistry::Claim(uint64_t schedule_id, uint64_t current_block) {
    auto it = schedules_.find(schedule_id);
    if (it == schedules_.end()) return 0;

    VestingSchedule& s = it->second;
    if (s.revoked) return 0;

    uint64_t claimable = GetClaimable(schedule_id, current_block);
    if (claimable == 0) return 0;

    s.claimed_amount += claimable;
    return claimable;
}

// ---------------------------------------------------------------------------
// Revocation
// ---------------------------------------------------------------------------

uint64_t VestingRegistry::Revoke(uint64_t schedule_id, uint64_t proposal_id,
                                  uint64_t current_block) {
    if (proposal_id == 0) return 0;

    auto it = schedules_.find(schedule_id);
    if (it == schedules_.end()) return 0;

    VestingSchedule& s = it->second;
    if (s.revoked) return 0;

    uint64_t reclaimable = GetReclaimable(schedule_id, current_block);

    s.revoked          = true;
    s.revoked_at_block = current_block;

    return reclaimable;
}

uint64_t VestingRegistry::GetReclaimable(uint64_t schedule_id,
                                          uint64_t current_block) const {
    auto it = schedules_.find(schedule_id);
    if (it == schedules_.end()) return 0;
    const VestingSchedule& s = it->second;
    if (s.revoked) return 0;

    uint64_t vested = ComputeVested(s, current_block);
    return (s.total_amount > vested) ? s.total_amount - vested : 0;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

std::optional<VestingRegistry::VestingSchedule>
VestingRegistry::GetSchedule(uint64_t schedule_id) const {
    auto it = schedules_.find(schedule_id);
    if (it == schedules_.end()) return std::nullopt;
    return it->second;
}

std::vector<VestingRegistry::VestingSchedule>
VestingRegistry::GetSchedulesForBeneficiary(
        const std::vector<uint8_t>& beneficiary) const {
    std::vector<VestingSchedule> result;
    for (const auto& [id, s] : schedules_) {
        if (s.beneficiary == beneficiary) result.push_back(s);
    }
    return result;
}

}  // namespace governance
}  // namespace parthenon
