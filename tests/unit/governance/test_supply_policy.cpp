// ParthenonChain – SupplyPolicy Unit Tests
//
// Validates the 5 % / 10 % / 50 % supply-bonded threshold constants and
// all helper predicates for each of the three native assets.

#include "governance/supply_policy.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

void TestAbsoluteThresholds() {
    std::cout << "Test: Pre-computed absolute thresholds are correct" << std::endl;

    // TALN: max = 21 000 000 * 1e8 = 2 100 000 000 000 000
    assert(SupplyPolicy::TALN_MAX_SUPPLY == 2'100'000'000'000'000ULL);
    assert(SupplyPolicy::TALN_TIER_LOW   ==   105'000'000'000'000ULL);  //  5 %
    assert(SupplyPolicy::TALN_TIER_MID   ==   210'000'000'000'000ULL);  // 10 %
    assert(SupplyPolicy::TALN_TIER_HIGH  == 1'050'000'000'000'000ULL);  // 50 %

    // DRM: max = 41 000 000 * 1e8 = 4 100 000 000 000 000
    assert(SupplyPolicy::DRM_MAX_SUPPLY == 4'100'000'000'000'000ULL);
    assert(SupplyPolicy::DRM_TIER_LOW   ==   205'000'000'000'000ULL);
    assert(SupplyPolicy::DRM_TIER_MID   ==   410'000'000'000'000ULL);
    assert(SupplyPolicy::DRM_TIER_HIGH  == 2'050'000'000'000'000ULL);

    // OBL: max = 61 000 000 * 1e8 = 6 100 000 000 000 000
    assert(SupplyPolicy::OBL_MAX_SUPPLY == 6'100'000'000'000'000ULL);
    assert(SupplyPolicy::OBL_TIER_LOW   ==   305'000'000'000'000ULL);
    assert(SupplyPolicy::OBL_TIER_MID   ==   610'000'000'000'000ULL);
    assert(SupplyPolicy::OBL_TIER_HIGH  == 3'050'000'000'000'000ULL);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTierBps() {
    std::cout << "Test: Tier basis-point constants" << std::endl;
    assert(SupplyPolicy::TIER_LOW_BPS  ==  500);
    assert(SupplyPolicy::TIER_MID_BPS  == 1000);
    assert(SupplyPolicy::TIER_HIGH_BPS == 5000);
    std::cout << "  \u2713 Passed" << std::endl;
}

void TestComputeThreshold() {
    std::cout << "Test: ComputeThreshold arithmetic" << std::endl;

    // Edge case: supply < 10000 – was previously losing all precision
    // supply=9999, bps=5000 → should be 4999 (not 0)
    assert(SupplyPolicy::ComputeThreshold(9999, 5000) == 4999);

    // 10 000 supply, 5 % = 500
    assert(SupplyPolicy::ComputeThreshold(10000, 500) == 500);

    // 10 000 supply, 10 % = 1 000
    assert(SupplyPolicy::ComputeThreshold(10000, 1000) == 1000);

    // 10 000 supply, 50 % = 5 000
    assert(SupplyPolicy::ComputeThreshold(10000, 5000) == 5000);

    // Zero supply
    assert(SupplyPolicy::ComputeThreshold(0, 5000) == 0);

    // Full TALN supply, 5 %  →  TALN_TIER_LOW
    assert(SupplyPolicy::ComputeThreshold(SupplyPolicy::TALN_MAX_SUPPLY,
                                          SupplyPolicy::TIER_LOW_BPS)
           == SupplyPolicy::TALN_TIER_LOW);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestIsBondingHealthy() {
    std::cout << "Test: IsBondingHealthy" << std::endl;

    uint64_t total = 1'000'000;

    // 50 000 / 1 000 000 = 5 % → exactly at minimum → healthy
    assert( SupplyPolicy::IsBondingHealthy(50000, total));

    // 49 999 / 1 000 000 < 5 % → unhealthy
    assert(!SupplyPolicy::IsBondingHealthy(49999, total));

    // 100 000 / 1 000 000 = 10 % → healthy
    assert( SupplyPolicy::IsBondingHealthy(100000, total));

    // Zero total supply → always unhealthy
    assert(!SupplyPolicy::IsBondingHealthy(1, 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestExceedsTreasuryCap() {
    std::cout << "Test: ExceedsTreasuryCap (50 % of total supply)" << std::endl;

    uint64_t total = 1'000'000;
    uint64_t cap   = 500'000;  // 50 % of 1 000 000

    // Exactly at cap: not exceeded
    assert(!SupplyPolicy::ExceedsTreasuryCap(cap, 0, total));

    // One over the cap: exceeded
    assert( SupplyPolicy::ExceedsTreasuryCap(cap, 1, total));

    // Treasury at 300 000 + deposit 200 000 = 500 000 → exactly at cap → not exceeded
    assert(!SupplyPolicy::ExceedsTreasuryCap(300000, 200000, total));

    // Treasury at 300 000 + deposit 200 001 → exceeded
    assert( SupplyPolicy::ExceedsTreasuryCap(300000, 200001, total));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestIsWhale() {
    std::cout << "Test: IsWhale (10 % threshold)" << std::endl;

    uint64_t total = 10'000;

    // Exactly 10 % (1 000) → whale
    assert( SupplyPolicy::IsWhale(1000, total));

    // Just below 10 % (999) → not a whale
    assert(!SupplyPolicy::IsWhale(999, total));

    // 50 % → definitely a whale
    assert( SupplyPolicy::IsWhale(5000, total));

    // Zero total supply → not a whale (avoid /0)
    assert(!SupplyPolicy::IsWhale(1, 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestComputeBondedQuorum() {
    std::cout << "Test: ComputeBondedQuorum = 5 % of bonded supply" << std::endl;

    // 5 % of 2 000 000 = 100 000
    assert(SupplyPolicy::ComputeBondedQuorum(2'000'000) == 100'000);

    // 5 % of 0 = 0
    assert(SupplyPolicy::ComputeBondedQuorum(0) == 0);

    // Against TALN staked supply: 5 % of 1 050 000 TALN base units
    uint64_t staked = 1'050'000ULL * SupplyPolicy::BASE_UNIT;
    uint64_t quorum = SupplyPolicy::ComputeBondedQuorum(staked);
    // should equal 52 500 TALN in base units
    assert(quorum == 52'500ULL * SupplyPolicy::BASE_UNIT);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "SupplyPolicy Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestAbsoluteThresholds();
        TestTierBps();
        TestComputeThreshold();
        TestIsBondingHealthy();
        TestExceedsTreasuryCap();
        TestIsWhale();
        TestComputeBondedQuorum();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All SupplyPolicy tests passed! \u2713" << std::endl;
        std::cout << "=============================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
