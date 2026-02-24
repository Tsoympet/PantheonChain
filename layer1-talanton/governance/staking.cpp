#include "staking.h"

namespace parthenon {
namespace governance {

StakingRegistry::StakingRegistry(uint64_t unstake_cooldown_blocks)
    : unstake_cooldown_(unstake_cooldown_blocks) {}

bool StakingRegistry::Stake(const std::vector<uint8_t>& address,
                             uint64_t amount, uint64_t lock_period,
                             uint64_t block_height) {
    if (address.empty() || amount == 0) return false;

    auto& rec = stakes_[address];
    if (rec.address.empty()) {
        rec.address        = address;
        rec.staked_at_block = block_height;
        rec.pending_unstake = 0;
    }
    rec.staked_amount += amount;

    // Extend lock if the new lock period pushes it further out
    uint64_t new_lock = block_height + lock_period;
    if (new_lock > rec.locked_until_block) {
        rec.locked_until_block = new_lock;
    }
    return true;
}

bool StakingRegistry::RequestUnstake(const std::vector<uint8_t>& address,
                                      uint64_t amount, uint64_t block_height) {
    auto it = stakes_.find(address);
    if (it == stakes_.end()) return false;

    StakeRecord& rec = it->second;
    if (IsStakeLocked(address, block_height)) return false;

    uint64_t available = rec.staked_amount - rec.pending_unstake;
    if (amount == 0 || amount > available) return false;

    // Only one pending unstake request per address at a time
    if (unstake_requests_.count(address)) {
        auto& req = unstake_requests_[address];
        if (req.status == UnstakeStatus::PENDING) return false;
    }

    rec.pending_unstake += amount;

    UnstakeRequest req;
    req.address            = address;
    req.amount             = amount;
    req.requested_at_block = block_height;
    req.claimable_at_block = block_height + unstake_cooldown_;
    req.status             = UnstakeStatus::PENDING;
    unstake_requests_[address] = req;
    return true;
}

bool StakingRegistry::ClaimUnstake(const std::vector<uint8_t>& address,
                                    uint64_t block_height) {
    auto req_it = unstake_requests_.find(address);
    if (req_it == unstake_requests_.end()) return false;

    UnstakeRequest& req = req_it->second;
    if (req.status != UnstakeStatus::PENDING) return false;
    if (block_height < req.claimable_at_block) return false;

    auto stake_it = stakes_.find(address);
    if (stake_it == stakes_.end()) return false;

    StakeRecord& rec = stake_it->second;
    if (rec.staked_amount < req.amount) return false;

    rec.staked_amount  -= req.amount;
    rec.pending_unstake -= req.amount;
    req.status          = UnstakeStatus::CLAIMED;
    return true;
}

bool StakingRegistry::Slash(const std::vector<uint8_t>& address,
                             uint64_t amount, const std::string& reason,
                             uint64_t block_height) {
    if (amount == 0) return false;
    auto it = stakes_.find(address);
    if (it == stakes_.end()) return false;

    StakeRecord& rec = it->second;
    if (amount > rec.staked_amount) return false;

    rec.staked_amount -= amount;
    // Also reduce pending_unstake proportionally if needed
    if (rec.pending_unstake > rec.staked_amount) {
        rec.pending_unstake = rec.staked_amount;
    }

    SlashRecord sr;
    sr.address      = address;
    sr.amount       = amount;
    sr.reason       = reason;
    sr.block_height = block_height;
    slash_history_.push_back(sr);
    return true;
}

uint64_t StakingRegistry::GetStake(const std::vector<uint8_t>& address) const {
    auto it = stakes_.find(address);
    return (it != stakes_.end()) ? it->second.staked_amount : 0;
}

uint64_t StakingRegistry::GetVotingPower(const std::vector<uint8_t>& address) const {
    auto it = stakes_.find(address);
    if (it == stakes_.end()) return 0;
    const StakeRecord& rec = it->second;
    return rec.staked_amount - rec.pending_unstake;
}

bool StakingRegistry::IsStakeLocked(const std::vector<uint8_t>& address,
                                     uint64_t block_height) const {
    auto it = stakes_.find(address);
    if (it == stakes_.end()) return false;
    return block_height < it->second.locked_until_block;
}

std::optional<StakingRegistry::StakeRecord>
StakingRegistry::GetStakeRecord(const std::vector<uint8_t>& address) const {
    auto it = stakes_.find(address);
    if (it == stakes_.end()) return std::nullopt;
    return it->second;
}

std::optional<StakingRegistry::UnstakeRequest>
StakingRegistry::GetUnstakeRequest(const std::vector<uint8_t>& address) const {
    auto it = unstake_requests_.find(address);
    if (it == unstake_requests_.end()) return std::nullopt;
    return it->second;
}

uint64_t StakingRegistry::GetTotalStaked() const {
    uint64_t total = 0;
    for (const auto& [addr, rec] : stakes_) total += rec.staked_amount;
    return total;
}

uint64_t StakingRegistry::GetTotalVotingPower() const {
    uint64_t total = 0;
    for (const auto& [addr, rec] : stakes_)
        total += (rec.staked_amount - rec.pending_unstake);
    return total;
}

std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
StakingRegistry::GetAllVotingPowers() const {
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> result;
    result.reserve(stakes_.size());
    for (const auto& [addr, rec] : stakes_) {
        uint64_t power = GetVotingPower(addr);
        if (power > 0) {
            result.emplace_back(addr, power);
        }
    }
    return result;
}

const std::vector<StakingRegistry::SlashRecord>&
StakingRegistry::GetSlashHistory() const {
    return slash_history_;
}

}  // namespace governance
}  // namespace parthenon
