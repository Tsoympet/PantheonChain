#include "staking.h"

namespace parthenon {
namespace governance {

StakingRegistry::StakingRegistry(uint64_t unstake_cooldown_blocks)
    : unstake_cooldown_(unstake_cooldown_blocks) {}

bool StakingRegistry::Stake(const std::vector<uint8_t>& /*address*/,
                             uint64_t /*amount*/, uint64_t /*lock_period*/,
                             uint64_t /*block_height*/) {
    // Staking is disabled.  Voting power is now derived directly from token
    // balances via BalanceVotingRegistry.  See governance/balance_voting.h.
    return false;
}

bool StakingRegistry::RequestUnstake(const std::vector<uint8_t>& /*address*/,
                                      uint64_t /*amount*/, uint64_t /*block_height*/) {
    // Staking is disabled; unstaking is a no-op.
    return false;
}

bool StakingRegistry::ClaimUnstake(const std::vector<uint8_t>& /*address*/,
                                    uint64_t /*block_height*/) {
    // Staking is disabled; nothing to claim.
    return false;
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
