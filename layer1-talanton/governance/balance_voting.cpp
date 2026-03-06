// ParthenonChain - One-Address-One-Vote Registry Implementation
//
// Every token holder gets exactly 1 vote regardless of balance size.

#include "balance_voting.h"

namespace parthenon {
namespace governance {

void BalanceVotingRegistry::SetBalances(
        const std::map<std::vector<uint8_t>, uint64_t>& balances) {
    balances_.clear();
    for (const auto& [addr, amount] : balances) {
        if (!addr.empty() && amount > 0) {
            balances_[addr] = amount;
        }
    }
}

void BalanceVotingRegistry::UpdateBalance(const std::vector<uint8_t>& address,
                                          uint64_t amount) {
    if (address.empty()) return;
    if (amount == 0) {
        balances_.erase(address);
    } else {
        balances_[address] = amount;
    }
}

uint64_t BalanceVotingRegistry::GetVotingPower(
        const std::vector<uint8_t>& address) const {
    // One-address-one-vote: any positive balance → power = 1.
    return (balances_.count(address) > 0) ? 1u : 0u;
}

uint64_t BalanceVotingRegistry::GetTotalVotingPower() const {
    // Total eligible voters = number of addresses with a positive balance.
    return static_cast<uint64_t>(balances_.size());
}

std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
BalanceVotingRegistry::GetAllVotingPowers() const {
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> result;
    result.reserve(balances_.size());
    for (const auto& [addr, amount] : balances_) {
        // Emit (address, 1) for every holder — balance magnitude is irrelevant.
        result.emplace_back(addr, 1u);
    }
    return result;
}

}  // namespace governance
}  // namespace parthenon
