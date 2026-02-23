#include "supply_policy.h"

namespace parthenon {
namespace governance {

uint64_t SupplyPolicy::ComputeThreshold(uint64_t supply, uint32_t bps) {
    // Two-term formula avoids both overflow (supply * bps can exceed uint64_t)
    // and precision loss from integer truncation:
    //   result = (supply / 10000) * bps + (supply % 10000) * bps / 10000
    // The remainder term ensures rounding is correct for all supply values,
    // including small test values where supply < 10000.
    uint64_t quotient  = supply / 10000ULL;
    uint64_t remainder = supply % 10000ULL;
    return quotient * bps + (remainder * bps) / 10000ULL;
}

bool SupplyPolicy::IsBondingHealthy(uint64_t bonded_supply,
                                    uint64_t total_supply,
                                    uint32_t min_bps) {
    if (total_supply == 0) return false;
    // bonded / total >= min_bps / 10000
    // ↔ bonded * 10000 >= total * min_bps
    return bonded_supply * 10000ULL >= total_supply * static_cast<uint64_t>(min_bps);
}

bool SupplyPolicy::ExceedsTreasuryCap(uint64_t treasury_balance,
                                      uint64_t deposit,
                                      uint64_t total_supply) {
    uint64_t cap = ComputeThreshold(total_supply, TIER_HIGH_BPS);
    // Overflow-safe addition
    if (deposit > UINT64_MAX - treasury_balance) return true;
    return (treasury_balance + deposit) > cap;
}

bool SupplyPolicy::IsWhale(uint64_t raw_power, uint64_t total_supply) {
    if (total_supply == 0) return false;
    // raw_power / total >= TIER_MID_BPS / 10000
    // ↔ raw_power * 10000 >= total * TIER_MID_BPS
    return raw_power * 10000ULL >= total_supply * static_cast<uint64_t>(TIER_MID_BPS);
}

uint64_t SupplyPolicy::ComputeBondedQuorum(uint64_t bonded_supply) {
    return ComputeThreshold(bonded_supply, TIER_LOW_BPS);
}

}  // namespace governance
}  // namespace parthenon
