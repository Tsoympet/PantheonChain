// ParthenonChain - Balance-Based Voting Registry Implementation

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
    auto it = balances_.find(address);
    return (it != balances_.end()) ? it->second : 0;
}

uint64_t BalanceVotingRegistry::GetTotalVotingPower() const {
    uint64_t total = 0;
    for (const auto& [addr, amount] : balances_) {
        total += amount;
    }
    return total;
}

std::vector<std::pair<std::vector<uint8_t>, uint64_t>>
BalanceVotingRegistry::GetAllVotingPowers() const {
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> result;
    result.reserve(balances_.size());
    for (const auto& [addr, amount] : balances_) {
        if (amount > 0) {
            result.emplace_back(addr, amount);
        }
    }
    return result;
}

}  // namespace governance
}  // namespace parthenon
