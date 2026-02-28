#include "antiwhale.h"

#include <stdexcept>

namespace parthenon {
namespace governance {

AntiWhaleGuard::AntiWhaleGuard(const Config& cfg) : config_(cfg) {}

uint64_t AntiWhaleGuard::Isqrt(uint64_t n) {
    if (n == 0) return 0;
    uint64_t x = n;
    uint64_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

uint64_t AntiWhaleGuard::ComputeEffectivePower(uint64_t raw_power,
                                               uint64_t /*total_supply*/) const {
    uint64_t power = raw_power;

    // 1. Quadratic voting
    if (config_.quadratic_voting_enabled) {
        power = Isqrt(power);
    }

    // 2. Hard cap
    if (config_.max_voting_power_cap > 0 && power > config_.max_voting_power_cap) {
        power = config_.max_voting_power_cap;
    }

    return power;
}

bool AntiWhaleGuard::IsWhale(uint64_t raw_power, uint64_t total_supply) const {
    if (total_supply == 0 || config_.whale_threshold_bps == 0) {
        return false;
    }
    // raw_power / total_supply > whale_threshold_bps / 10000
    // Rearranged: raw_power * 10000 > total_supply * whale_threshold_bps
    // Guard against uint64_t overflow on both sides.
    const uint64_t lhs = (raw_power <= UINT64_MAX / 10000)
                             ? raw_power * 10000
                             : UINT64_MAX;
    const uint64_t rhs = (config_.whale_threshold_bps == 0 ||
                          total_supply <= UINT64_MAX / config_.whale_threshold_bps)
                             ? total_supply * config_.whale_threshold_bps
                             : UINT64_MAX;
    return lhs > rhs;
}

}  // namespace governance
}  // namespace parthenon
