#include "treasury.h"

#include <algorithm>
#include <numeric>

namespace parthenon {
namespace governance {

Treasury::Treasury(uint32_t multisig_required, uint64_t reserve_ratio_bps)
    : multisig_required_(multisig_required),
      reserve_ratio_bps_(reserve_ratio_bps) {
    // Initialise all track balances to zero
    balances_[Track::CORE_DEVELOPMENT] = 0;
    balances_[Track::GRANTS]           = 0;
    balances_[Track::OPERATIONS]       = 0;
    balances_[Track::EMERGENCY]        = 0;
    balances_[Track::UNCATEGORIZED]    = 0;
}

// ---------------------------------------------------------------------------
// Guardians
// ---------------------------------------------------------------------------

bool Treasury::AddGuardian(const std::vector<uint8_t>& address) {
    if (address.empty()) return false;
    if (IsGuardian(address)) return false;
    guardians_.push_back(address);
    return true;
}

bool Treasury::RemoveGuardian(const std::vector<uint8_t>& address) {
    auto it = std::find(guardians_.begin(), guardians_.end(), address);
    if (it == guardians_.end()) return false;
    guardians_.erase(it);
    return true;
}

bool Treasury::IsGuardian(const std::vector<uint8_t>& address) const {
    return std::find(guardians_.begin(), guardians_.end(), address) != guardians_.end();
}

std::vector<std::vector<uint8_t>> Treasury::GetGuardians() const {
    return guardians_;
}

// ---------------------------------------------------------------------------
// Deposits
// ---------------------------------------------------------------------------

bool Treasury::Deposit(uint64_t amount, const std::vector<uint8_t>& from,
                       Track track, uint64_t block_height) {
    if (amount == 0) return false;
    balances_[track] += amount;
    RecordTx(true, amount, from, track, 0, 0, "deposit", block_height);
    return true;
}

// ---------------------------------------------------------------------------
// Reserve ratio helper
// ---------------------------------------------------------------------------

bool Treasury::ViolatesReserve(uint64_t debit_non_emergency) const {
    if (reserve_ratio_bps_ == 0) return false;

    uint64_t total = GetTotalBalance();
    if (total <= debit_non_emergency) return true;  // would empty treasury

    uint64_t after_total    = total - debit_non_emergency;
    uint64_t emergency_bal  = GetReserveBalance();
    // emergency must remain >= reserve_ratio_bps_ / 10000 of after_total
    // i.e. emergency_bal * 10000 >= after_total * reserve_ratio_bps_
    return (emergency_bal * 10000 < after_total * reserve_ratio_bps_);
}

// ---------------------------------------------------------------------------
// Single-track spending
// ---------------------------------------------------------------------------

bool Treasury::Spend(uint64_t amount, const std::vector<uint8_t>& to,
                     uint64_t proposal_id, Track track,
                     const std::string& purpose, uint64_t block_height) {
    if (proposal_id == 0) return false;
    if (amount == 0) return false;
    if (track == Track::EMERGENCY) return false;  // use multi-sig for emergency

    auto& bal = balances_[track];
    if (amount > bal) return false;

    if (!IsWithinBudget(track, amount, block_height)) return false;
    if (ViolatesReserve(amount)) return false;

    bal -= amount;

    // Update active budget period spending
    for (auto& bp : budget_periods_) {
        if (block_height >= bp.start_block && block_height <= bp.end_block) {
            bp.track_spent[track] += amount;
            break;
        }
    }

    RecordTx(false, amount, to, track, proposal_id, 0, purpose, block_height);
    return true;
}

// ---------------------------------------------------------------------------
// Multi-sig spending (EMERGENCY track)
// ---------------------------------------------------------------------------

uint64_t Treasury::ProposeMultiSigSpend(uint64_t amount,
                                        const std::vector<uint8_t>& to,
                                        const std::string& purpose,
                                        const std::vector<uint8_t>& initiator,
                                        uint64_t block_height) {
    if (amount == 0 || to.empty()) return 0;
    if (!IsGuardian(initiator)) return 0;

    MultiSigSpend spend;
    spend.spend_id        = next_spend_id_++;
    spend.amount          = amount;
    spend.recipient       = to;
    spend.purpose         = purpose;
    spend.initiator       = initiator;
    spend.created_at_block = block_height;
    spend.executed        = false;
    spend.executed_at_block = 0;
    spend.signers.insert(initiator);  // initiator counts as first signature

    multisig_spends_[spend.spend_id] = spend;
    return spend.spend_id;
}

bool Treasury::SignMultiSigSpend(uint64_t spend_id,
                                 const std::vector<uint8_t>& guardian) {
    if (!IsGuardian(guardian)) return false;

    auto it = multisig_spends_.find(spend_id);
    if (it == multisig_spends_.end()) return false;
    if (it->second.executed) return false;

    it->second.signers.insert(guardian);
    return true;
}

bool Treasury::HasSufficientSignatures(uint64_t spend_id) const {
    auto it = multisig_spends_.find(spend_id);
    if (it == multisig_spends_.end()) return false;
    return static_cast<uint32_t>(it->second.signers.size()) >= multisig_required_;
}

bool Treasury::ExecuteMultiSigSpend(uint64_t spend_id, uint64_t block_height) {
    auto it = multisig_spends_.find(spend_id);
    if (it == multisig_spends_.end()) return false;
    if (it->second.executed) return false;
    if (!HasSufficientSignatures(spend_id)) return false;

    MultiSigSpend& spend = it->second;
    auto& em_bal = balances_[Track::EMERGENCY];
    if (spend.amount > em_bal) return false;

    em_bal -= spend.amount;
    spend.executed          = true;
    spend.executed_at_block = block_height;

    RecordTx(false, spend.amount, spend.recipient, Track::EMERGENCY,
             0, 0, spend.purpose, block_height);
    return true;
}

std::optional<Treasury::MultiSigSpend>
Treasury::GetMultiSigSpend(uint64_t spend_id) const {
    auto it = multisig_spends_.find(spend_id);
    if (it == multisig_spends_.end()) return std::nullopt;
    return it->second;
}

// ---------------------------------------------------------------------------
// Budget periods
// ---------------------------------------------------------------------------

uint64_t Treasury::CreateBudgetPeriod(uint64_t start_block, uint64_t end_block,
                                       const std::map<Track, uint64_t>& limits,
                                       uint64_t proposal_id) {
    if (proposal_id == 0) return 0;
    if (end_block <= start_block) return 0;

    BudgetPeriod bp;
    bp.period_id    = next_period_id_++;
    bp.start_block  = start_block;
    bp.end_block    = end_block;
    bp.track_limits = limits;
    budget_periods_.push_back(bp);
    return bp.period_id;
}

bool Treasury::IsWithinBudget(Track track, uint64_t amount,
                               uint64_t block_height) const {
    for (const auto& bp : budget_periods_) {
        if (block_height < bp.start_block || block_height > bp.end_block) continue;

        auto lim_it = bp.track_limits.find(track);
        if (lim_it == bp.track_limits.end() || lim_it->second == 0) return true;

        auto sp_it = bp.track_spent.find(track);
        uint64_t spent = (sp_it != bp.track_spent.end()) ? sp_it->second : 0;
        return (spent + amount) <= lim_it->second;
    }
    return true;  // No active budget period → no limit
}

std::optional<Treasury::BudgetPeriod>
Treasury::GetActiveBudgetPeriod(uint64_t block_height) const {
    for (const auto& bp : budget_periods_) {
        if (block_height >= bp.start_block && block_height <= bp.end_block)
            return bp;
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Milestone grants
// ---------------------------------------------------------------------------

uint64_t Treasury::CreateGrant(uint64_t proposal_id,
                                const std::vector<uint8_t>& recipient,
                                const std::string& purpose,
                                const std::vector<std::pair<std::string, uint64_t>>& milestones,
                                uint64_t block_height,
                                uint64_t vesting_cliff_blocks,
                                uint64_t vesting_duration_blocks) {
    if (proposal_id == 0 || recipient.empty() || milestones.empty()) return 0;

    uint64_t total = 0;
    for (const auto& m : milestones) total += m.second;
    if (total == 0) return 0;
    if (total > balances_[Track::GRANTS]) return 0;

    Grant grant;
    grant.grant_id        = next_grant_id_++;
    grant.proposal_id     = proposal_id;
    grant.recipient       = recipient;
    grant.purpose         = purpose;
    grant.total_amount    = total;
    grant.released_amount = 0;
    grant.revoked         = false;
    grant.created_at_block = block_height;
    for (const auto& [desc, amt] : milestones) {
        grant.milestones.emplace_back(desc, amt);
    }

    // Reserve the total amount from the GRANTS track balance
    balances_[Track::GRANTS] -= total;
    grants_[grant.grant_id] = grant;

    // Optionally create a cliff+linear vesting schedule so the recipient
    // cannot withdraw the entire grant immediately.
    if (vesting_registry_ != nullptr && vesting_duration_blocks > 0) {
        vesting_registry_->CreateSchedule(recipient, total, block_height,
                                          vesting_cliff_blocks,
                                          vesting_duration_blocks,
                                          grant.grant_id);
    }

    return grant.grant_id;
}

bool Treasury::ReleaseMilestone(uint64_t grant_id, uint32_t milestone_index,
                                 uint64_t proposal_id, uint64_t block_height) {
    if (proposal_id == 0) return false;

    auto it = grants_.find(grant_id);
    if (it == grants_.end()) return false;

    Grant& grant = it->second;
    if (grant.revoked) return false;
    if (milestone_index >= grant.milestones.size()) return false;

    Milestone& ms = grant.milestones[milestone_index];
    if (ms.released) return false;

    ms.released         = true;
    ms.released_at_block = block_height;
    grant.released_amount += ms.amount;

    RecordTx(false, ms.amount, grant.recipient, Track::GRANTS,
             proposal_id, grant_id, grant.purpose, block_height);
    return true;
}

bool Treasury::RevokeGrant(uint64_t grant_id, uint64_t proposal_id,
                            uint64_t block_height) {
    if (proposal_id == 0) return false;

    auto it = grants_.find(grant_id);
    if (it == grants_.end()) return false;

    Grant& grant = it->second;
    if (grant.revoked) return false;

    // Refund unreleased milestone amounts to GRANTS track
    uint64_t refund = 0;
    for (const auto& ms : grant.milestones) {
        if (!ms.released) refund += ms.amount;
    }
    balances_[Track::GRANTS] += refund;
    grant.revoked = true;

    RecordTx(true, refund, grant.recipient, Track::GRANTS,
             proposal_id, grant_id, "grant revoked – refund", block_height);
    return true;
}

std::optional<Treasury::Grant> Treasury::GetGrant(uint64_t grant_id) const {
    auto it = grants_.find(grant_id);
    if (it == grants_.end()) return std::nullopt;
    return it->second;
}

// ---------------------------------------------------------------------------
// Balance queries
// ---------------------------------------------------------------------------

uint64_t Treasury::GetTotalBalance() const {
    uint64_t total = 0;
    for (const auto& [t, b] : balances_) total += b;
    return total;
}

uint64_t Treasury::GetTrackBalance(Track track) const {
    auto it = balances_.find(track);
    return (it != balances_.end()) ? it->second : 0;
}

uint64_t Treasury::GetReserveBalance() const {
    return GetTrackBalance(Track::EMERGENCY);
}

// ---------------------------------------------------------------------------
// Audit log
// ---------------------------------------------------------------------------

const std::vector<Treasury::TxRecord>& Treasury::GetTransactions() const {
    return transactions_;
}

std::vector<Treasury::TxRecord>
Treasury::GetTransactionsByTrack(Track track) const {
    std::vector<TxRecord> result;
    for (const auto& tx : transactions_) {
        if (tx.track == track) result.push_back(tx);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Treasury::RecordTx(bool is_deposit, uint64_t amount,
                         const std::vector<uint8_t>& address,
                         Track track, uint64_t proposal_id,
                         uint64_t grant_id, const std::string& purpose,
                         uint64_t block_height) {
    TxRecord tx;
    tx.tx_id        = next_tx_id_++;
    tx.is_deposit   = is_deposit;
    tx.amount       = amount;
    tx.address      = address;
    tx.track        = track;
    tx.proposal_id  = proposal_id;
    tx.grant_id     = grant_id;
    tx.purpose      = purpose;
    tx.block_height = block_height;
    transactions_.push_back(tx);
}

}  // namespace governance
}  // namespace parthenon
