#ifndef PARTHENON_GOVERNANCE_SUPPLY_POLICY_H
#define PARTHENON_GOVERNANCE_SUPPLY_POLICY_H

#include <cstdint>

namespace parthenon {
namespace governance {

/**
 * SupplyPolicy
 *
 * Defines the three canonical bonded-supply tiers for PantheonChain
 * governance, expressed as percentages of each asset's hard max supply cap.
 *
 *  TIER_LOW   –  5 % of max supply
 *      Minimum governance-participation threshold.  When fewer than 5 % of
 *      tokens are participating (staked + delegated) governance actions lack
 *      legitimacy.  Also used as the QUORUM floor for standard proposals.
 *
 *  TIER_MID   – 10 % of max supply
 *      Anti-whale / single-entity influence ceiling.  Any address holding
 *      or controlling ≥ 10 % is classified as a whale; quadratic voting and
 *      the hard-cap in AntiWhaleGuard should be calibrated to this boundary.
 *
 *  TIER_HIGH  – 50 % of max supply
 *      Treasury hard cap.  The governance treasury must not accumulate more
 *      than 50 % of any asset's total supply — doing so would concentrate too
 *      much economic power in a single governance-controlled account.
 *
 * Per-asset absolute values (base units = whole_tokens × 100_000_000):
 *
 *  Asset   Max Supply     5 % tier          10 % tier         50 % tier
 *  ─────── ────────────── ───────────────── ───────────────── ─────────────────
 *  TALN    21 000 000     1 050 000 TALN    2 100 000 TALN    10 500 000 TALN
 *  DRM     41 000 000     2 050 000 DRM     4 100 000 DRM     20 500 000 DRM
 *  OBL     61 000 000     3 050 000 OBL     6 100 000 OBL     30 500 000 OBL
 *
 * All values are in base units (8 decimal places, same as Bitcoin satoshi).
 */
class SupplyPolicy {
  public:
    // ------------------------------------------------------------------ //
    //  Base supply constants (base units = whole tokens × 1e8)            //
    // ------------------------------------------------------------------ //

    static constexpr uint64_t BASE_UNIT         = 100'000'000ULL;

    static constexpr uint64_t TALN_MAX_SUPPLY   =  21'000'000ULL * BASE_UNIT;
    static constexpr uint64_t DRM_MAX_SUPPLY    =  41'000'000ULL * BASE_UNIT;
    static constexpr uint64_t OBL_MAX_SUPPLY    =  61'000'000ULL * BASE_UNIT;

    // ------------------------------------------------------------------ //
    //  Canonical governance tier basis-points                              //
    // ------------------------------------------------------------------ //

    static constexpr uint32_t TIER_LOW_BPS   =   500;   //  5 %
    static constexpr uint32_t TIER_MID_BPS   =  1000;   // 10 %
    static constexpr uint32_t TIER_HIGH_BPS  =  5000;   // 50 %

    // ------------------------------------------------------------------ //
    //  Pre-computed absolute thresholds (base units)                       //
    // ------------------------------------------------------------------ //

    // --- TALN ---
    static constexpr uint64_t TALN_TIER_LOW   = TALN_MAX_SUPPLY / 100 *  5;  //  1 050 000 TALN
    static constexpr uint64_t TALN_TIER_MID   = TALN_MAX_SUPPLY / 100 * 10;  //  2 100 000 TALN
    static constexpr uint64_t TALN_TIER_HIGH  = TALN_MAX_SUPPLY / 100 * 50;  // 10 500 000 TALN

    // --- DRM ---
    static constexpr uint64_t DRM_TIER_LOW    = DRM_MAX_SUPPLY  / 100 *  5;  //  2 050 000 DRM
    static constexpr uint64_t DRM_TIER_MID    = DRM_MAX_SUPPLY  / 100 * 10;  //  4 100 000 DRM
    static constexpr uint64_t DRM_TIER_HIGH   = DRM_MAX_SUPPLY  / 100 * 50;  // 20 500 000 DRM

    // --- OBL ---
    static constexpr uint64_t OBL_TIER_LOW    = OBL_MAX_SUPPLY  / 100 *  5;  //  3 050 000 OBL
    static constexpr uint64_t OBL_TIER_MID    = OBL_MAX_SUPPLY  / 100 * 10;  //  6 100 000 OBL
    static constexpr uint64_t OBL_TIER_HIGH   = OBL_MAX_SUPPLY  / 100 * 50;  // 30 500 000 OBL

    // ------------------------------------------------------------------ //
    //  Runtime helpers                                                      //
    // ------------------------------------------------------------------ //

    /**
     * Compute a threshold amount as `supply * bps / 10000`.
     * Safe against overflow for supply values up to ~1.8 × 10^19.
     */
    static uint64_t ComputeThreshold(uint64_t supply, uint32_t bps);

    /**
     * Is the bonded ratio healthy?
     * Returns true when bonded_supply / total_supply >= min_bps / 10000.
     * A healthy minimum is TIER_LOW_BPS (5 %).
     */
    static bool IsBondingHealthy(uint64_t bonded_supply,
                                 uint64_t total_supply,
                                 uint32_t min_bps = TIER_LOW_BPS);

    /**
     * Would adding `deposit` to the treasury exceed the 50 % supply cap?
     * Returns true when treasury_balance + deposit > TIER_HIGH threshold
     * of total_supply.  Pass the total circulating supply of the relevant
     * asset as total_supply.
     */
    static bool ExceedsTreasuryCap(uint64_t treasury_balance,
                                   uint64_t deposit,
                                   uint64_t total_supply);

    /**
     * Is raw_power a whale position relative to total_supply?
     * Uses TIER_MID_BPS (10 %) as the threshold.
     */
    static bool IsWhale(uint64_t raw_power, uint64_t total_supply);

    /**
     * Compute the minimum quorum for a proposal given the current bonded
     * (staked) supply.  Returns 5 % of bonded_supply — so quorum scales
     * down naturally as participation falls rather than being an immovable
     * absolute number.
     */
    static uint64_t ComputeBondedQuorum(uint64_t bonded_supply);
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_SUPPLY_POLICY_H
