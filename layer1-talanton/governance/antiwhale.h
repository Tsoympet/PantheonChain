#ifndef PARTHENON_GOVERNANCE_ANTIWHALE_H
#define PARTHENON_GOVERNANCE_ANTIWHALE_H

#include <cstdint>

namespace parthenon {
namespace governance {

/**
 * AntiWhaleGuard
 *
 * Prevents any single large holder ("whale") from dominating on-chain
 * governance.  Three independent, composable protections are provided:
 *
 *  1. Quadratic Voting  – effective power = floor(sqrt(raw_power)), so
 *     doubling tokens only adds ~41 % more influence instead of 100 %.
 *
 *  2. Hard Cap          – a single voter's effective power is capped at
 *     max_voting_power_cap (absolute token units).  0 = disabled.
 *
 *  3. Whale Threshold   – addresses whose raw_power/total_supply exceeds
 *     whale_threshold_bps (basis points) are identified as whales.  The
 *     caller can use IsWhale() to enforce additional off-chain or on-chain
 *     restrictions (e.g. only one whale proposal active at a time).
 *
 * All three protections are applied in the order listed above when
 * ComputeEffectivePower() is called.
 */
class AntiWhaleGuard {
  public:
    struct Config {
        bool     quadratic_voting_enabled;   // floor(sqrt(raw)) instead of raw
        uint64_t max_voting_power_cap;       // hard cap, 0 = no cap
        uint64_t whale_threshold_bps;        // basis points of total_supply; 0 = disabled
    };

    static constexpr Config kDefaultConfig{
        /*quadratic_voting_enabled=*/true,
        /*max_voting_power_cap=*/0,
        /*whale_threshold_bps=*/1000  // 10 % of supply = whale
    };

    explicit AntiWhaleGuard(const Config& cfg = kDefaultConfig);

    /**
     * Compute the effective voting power that will be counted in tallies.
     * total_supply is only used when quadratic voting is disabled (for the
     * hard-cap check against a percentage of supply).  Pass 0 if unknown.
     */
    uint64_t ComputeEffectivePower(uint64_t raw_power, uint64_t total_supply) const;

    /**
     * Returns true if raw_power / total_supply > whale_threshold_bps.
     * Returns false when total_supply == 0 or whale_threshold_bps == 0.
     */
    bool IsWhale(uint64_t raw_power, uint64_t total_supply) const;

    void       SetConfig(const Config& cfg) { config_ = cfg; }
    const Config& GetConfig() const { return config_; }

  private:
    Config config_;

    // Integer square root (floor).
    static uint64_t Isqrt(uint64_t n);
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_ANTIWHALE_H
