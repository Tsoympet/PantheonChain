#include "payments_state_machine.h"

namespace pantheon::drachma {

void PaymentsStateMachine::Credit(const std::string& account, uint64_t amount) {
    balances_[account] += amount;
}

PaymentResult PaymentsStateMachine::Transfer(const std::string& from, const std::string& to,
                                             uint64_t amount, uint64_t fee) {
    if (from.empty() || to.empty()) {
        return {false, "from/to account must be non-empty"};
    }
    if (from == to) {
        return {false, "self transfer is not allowed"};
    }

    const uint64_t debit = amount + fee;
    if (amount == 0 || debit < amount) {
        return {false, "invalid transfer amount"};
    }

    auto from_it = balances_.find(from);
    if (from_it == balances_.end() || from_it->second < debit) {
        return {false, "insufficient balance"};
    }

    from_it->second -= debit;
    balances_[to] += amount;
    collected_fees_ += fee;

    return {true, ""};
}

uint64_t PaymentsStateMachine::Balance(const std::string& account) const {
    const auto it = balances_.find(account);
    return it == balances_.end() ? 0 : it->second;
}

uint64_t PaymentsStateMachine::CollectedFees() const { return collected_fees_; }

}  // namespace pantheon::drachma
