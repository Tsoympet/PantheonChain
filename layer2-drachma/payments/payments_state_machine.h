#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace pantheon::drachma {

struct PaymentResult {
    bool ok = false;
    std::string reason;
};

class PaymentsStateMachine {
  public:
    void Credit(const std::string& account, uint64_t amount);

    PaymentResult Transfer(const std::string& from, const std::string& to, uint64_t amount,
                           uint64_t fee);

    [[nodiscard]] uint64_t Balance(const std::string& account) const;
    [[nodiscard]] uint64_t CollectedFees() const;

  private:
    std::unordered_map<std::string, uint64_t> balances_;
    uint64_t collected_fees_ = 0;
};

}  // namespace pantheon::drachma
