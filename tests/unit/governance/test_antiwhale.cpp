// ParthenonChain – AntiWhaleGuard Unit Tests

#include "governance/antiwhale.h"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace parthenon::governance;

void TestQuadraticVoting() {
    std::cout << "Test: Quadratic voting scales power as floor(sqrt(raw))" << std::endl;

    AntiWhaleGuard::Config cfg;
    cfg.quadratic_voting_enabled = true;
    cfg.max_voting_power_cap     = 0;
    cfg.whale_threshold_bps      = 0;
    AntiWhaleGuard g(cfg);

    assert(g.ComputeEffectivePower(0, 0)       == 0);
    assert(g.ComputeEffectivePower(1, 0)       == 1);
    assert(g.ComputeEffectivePower(4, 0)       == 2);
    assert(g.ComputeEffectivePower(9, 0)       == 3);
    assert(g.ComputeEffectivePower(100, 0)     == 10);
    assert(g.ComputeEffectivePower(10000, 0)   == 100);
    assert(g.ComputeEffectivePower(1000000, 0) == 1000);

    // A whale with 1 000 000 tokens only has 1 000 effective votes
    // while a small holder with 100 tokens has 10 – much better ratio
    uint64_t whale_raw  = 1000000;
    uint64_t normal_raw = 100;
    uint64_t whale_eff  = g.ComputeEffectivePower(whale_raw,  0);
    uint64_t normal_eff = g.ComputeEffectivePower(normal_raw, 0);
    // Without quadratic: whale would have 10000x more influence
    // With quadratic:    whale has 100x more influence – much fairer
    assert(whale_eff / normal_eff == 100);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestLinearVotingWithCap() {
    std::cout << "Test: Linear voting with hard cap" << std::endl;

    AntiWhaleGuard::Config cfg;
    cfg.quadratic_voting_enabled = false;
    cfg.max_voting_power_cap     = 500;
    cfg.whale_threshold_bps      = 0;
    AntiWhaleGuard g(cfg);

    assert(g.ComputeEffectivePower(100, 0) == 100);  // below cap
    assert(g.ComputeEffectivePower(500, 0) == 500);  // at cap
    assert(g.ComputeEffectivePower(999, 0) == 500);  // capped
    assert(g.ComputeEffectivePower(1e9,  0) == 500); // capped

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestQuadraticAndCap() {
    std::cout << "Test: Quadratic + hard cap combined" << std::endl;

    AntiWhaleGuard::Config cfg;
    cfg.quadratic_voting_enabled = true;
    cfg.max_voting_power_cap     = 50;   // cap after sqrt
    cfg.whale_threshold_bps      = 0;
    AntiWhaleGuard g(cfg);

    // sqrt(10000) = 100 → capped at 50
    assert(g.ComputeEffectivePower(10000, 0) == 50);
    // sqrt(4) = 2 → below cap
    assert(g.ComputeEffectivePower(4, 0) == 2);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestWhaleDetection() {
    std::cout << "Test: Whale detection" << std::endl;

    AntiWhaleGuard::Config cfg;
    cfg.quadratic_voting_enabled = false;
    cfg.max_voting_power_cap     = 0;
    cfg.whale_threshold_bps      = 1000;  // 10 %
    AntiWhaleGuard g(cfg);

    uint64_t supply = 1000000;

    // 10% exactly = NOT whale (strict >)
    assert(!g.IsWhale(100000, supply));

    // 10.01% = whale
    assert(g.IsWhale(100001, supply));

    // 5% = not whale
    assert(!g.IsWhale(50000, supply));

    // Zero supply → never whale
    assert(!g.IsWhale(999999, 0));

    // Disabled threshold → never whale
    cfg.whale_threshold_bps = 0;
    g.SetConfig(cfg);
    assert(!g.IsWhale(999999, supply));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestDefaultConfig() {
    std::cout << "Test: Default configuration" << std::endl;

    AntiWhaleGuard g;
    const auto& cfg = g.GetConfig();
    assert(cfg.quadratic_voting_enabled);
    assert(cfg.max_voting_power_cap == 0);
    assert(cfg.whale_threshold_bps  == 1000);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "AntiWhaleGuard Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestQuadraticVoting();
        TestLinearVotingWithCap();
        TestQuadraticAndCap();
        TestWhaleDetection();
        TestDefaultConfig();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All AntiWhaleGuard tests passed! \u2713" << std::endl;
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
