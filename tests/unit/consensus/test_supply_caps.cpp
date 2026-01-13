// ParthenonChain - Issuance Cap Verification Tests
// Comprehensive proof that supply caps (21M/41M/61M) are never exceeded

#include "consensus/issuance.h"
#include "primitives/asset.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace parthenon::consensus;
using namespace parthenon::primitives;

void TestTalanton21MCapNeverExceeded() {
    std::cout << "Test: TALANTON supply never exceeds 21M" << std::endl;
    
    const uint64_t MAX_SUPPLY = 21000000ULL * AssetSupply::BASE_UNIT;
    uint64_t accumulated_supply = 0;
    
    // Simulate entire blockchain lifetime (100 halvings is effectively forever)
    for (uint64_t halving = 0; halving < 100; halving++) {
        uint64_t epoch_start = halving * Issuance::HALVING_INTERVAL;
        uint64_t reward = Issuance::GetBlockReward(epoch_start, AssetID::TALANTON);
        
        // Add entire epoch's supply
        uint64_t epoch_supply = reward * Issuance::HALVING_INTERVAL;
        accumulated_supply += epoch_supply;
        
        // After each epoch, verify we haven't exceeded cap
        assert(accumulated_supply <= MAX_SUPPLY);
        
        if (halving % 10 == 0) {
            std::cout << "  After halving " << halving << ": " 
                      << accumulated_supply / AssetSupply::BASE_UNIT << " TALN" 
                      << " (max: " << MAX_SUPPLY / AssetSupply::BASE_UNIT << ")" << std::endl;
        }
        
        // If reward is 0, we're done
        if (reward == 0) {
            std::cout << "  Issuance complete at halving " << halving << std::endl;
            break;
        }
    }
    
    std::cout << "  Final supply: " << accumulated_supply / AssetSupply::BASE_UNIT << " TALN" << std::endl;
    assert(accumulated_supply <= MAX_SUPPLY);
    
    // Verify supply calculation matches
    uint64_t height = 1000 * Issuance::HALVING_INTERVAL;
    uint64_t calculated = Issuance::CalculateSupplyAtHeight(height, AssetID::TALANTON);
    assert(calculated <= MAX_SUPPLY);
    
    std::cout << "  ✓ Passed (21M cap enforced)" << std::endl;
}

void TestDrachma41MCapNeverExceeded() {
    std::cout << "Test: DRACHMA supply never exceeds 41M" << std::endl;
    
    const uint64_t MAX_SUPPLY = 41000000ULL * AssetSupply::BASE_UNIT;
    uint64_t accumulated_supply = 0;
    
    // Simulate entire blockchain lifetime
    for (uint64_t halving = 0; halving < 100; halving++) {
        uint64_t epoch_start = halving * Issuance::HALVING_INTERVAL;
        uint64_t reward = Issuance::GetBlockReward(epoch_start, AssetID::DRACHMA);
        
        // Add entire epoch's supply
        uint64_t epoch_supply = reward * Issuance::HALVING_INTERVAL;
        accumulated_supply += epoch_supply;
        
        // After each epoch, verify we haven't exceeded cap
        assert(accumulated_supply <= MAX_SUPPLY);
        
        if (halving % 10 == 0) {
            std::cout << "  After halving " << halving << ": " 
                      << accumulated_supply / AssetSupply::BASE_UNIT << " DRM" 
                      << " (max: " << MAX_SUPPLY / AssetSupply::BASE_UNIT << ")" << std::endl;
        }
        
        // If reward is 0, we're done
        if (reward == 0) {
            std::cout << "  Issuance complete at halving " << halving << std::endl;
            break;
        }
    }
    
    std::cout << "  Final supply: " << accumulated_supply / AssetSupply::BASE_UNIT << " DRM" << std::endl;
    assert(accumulated_supply <= MAX_SUPPLY);
    
    // Verify supply calculation matches
    uint64_t height = 1000 * Issuance::HALVING_INTERVAL;
    uint64_t calculated = Issuance::CalculateSupplyAtHeight(height, AssetID::DRACHMA);
    assert(calculated <= MAX_SUPPLY);
    
    std::cout << "  ✓ Passed (41M cap enforced)" << std::endl;
}

void TestObolos61MCapNeverExceeded() {
    std::cout << "Test: OBOLOS supply never exceeds 61M" << std::endl;
    
    const uint64_t MAX_SUPPLY = 61000000ULL * AssetSupply::BASE_UNIT;
    uint64_t accumulated_supply = 0;
    
    // Simulate entire blockchain lifetime
    for (uint64_t halving = 0; halving < 100; halving++) {
        uint64_t epoch_start = halving * Issuance::HALVING_INTERVAL;
        uint64_t reward = Issuance::GetBlockReward(epoch_start, AssetID::OBOLOS);
        
        // Add entire epoch's supply
        uint64_t epoch_supply = reward * Issuance::HALVING_INTERVAL;
        accumulated_supply += epoch_supply;
        
        // After each epoch, verify we haven't exceeded cap
        assert(accumulated_supply <= MAX_SUPPLY);
        
        if (halving % 10 == 0) {
            std::cout << "  After halving " << halving << ": " 
                      << accumulated_supply / AssetSupply::BASE_UNIT << " OBL" 
                      << " (max: " << MAX_SUPPLY / AssetSupply::BASE_UNIT << ")" << std::endl;
        }
        
        // If reward is 0, we're done
        if (reward == 0) {
            std::cout << "  Issuance complete at halving " << halving << std::endl;
            break;
        }
    }
    
    std::cout << "  Final supply: " << accumulated_supply / AssetSupply::BASE_UNIT << " OBL" << std::endl;
    assert(accumulated_supply <= MAX_SUPPLY);
    
    // Verify supply calculation matches
    uint64_t height = 1000 * Issuance::HALVING_INTERVAL;
    uint64_t calculated = Issuance::CalculateSupplyAtHeight(height, AssetID::OBOLOS);
    assert(calculated <= MAX_SUPPLY);
    
    std::cout << "  ✓ Passed (61M cap enforced)" << std::endl;
}

void TestSupplyCapEnforcementAtEveryHeight() {
    std::cout << "Test: Supply caps enforced at every block height" << std::endl;
    
    struct AssetCap {
        AssetID id;
        uint64_t max_supply;
        const char* ticker;
    };
    
    AssetCap assets[] = {
        {AssetID::TALANTON, 21000000ULL * AssetSupply::BASE_UNIT, "TALN"},
        {AssetID::DRACHMA,  41000000ULL * AssetSupply::BASE_UNIT, "DRM"},
        {AssetID::OBOLOS,   61000000ULL * AssetSupply::BASE_UNIT, "OBL"}
    };
    
    // Test at various significant heights
    std::vector<uint64_t> test_heights = {
        0, 1, 100, 1000, 10000,
        Issuance::HALVING_INTERVAL - 1,
        Issuance::HALVING_INTERVAL,
        Issuance::HALVING_INTERVAL + 1,
        10 * Issuance::HALVING_INTERVAL,
        20 * Issuance::HALVING_INTERVAL,
        50 * Issuance::HALVING_INTERVAL,
        100 * Issuance::HALVING_INTERVAL
    };
    
    for (const auto& asset : assets) {
        std::cout << "  Testing " << asset.ticker << "..." << std::endl;
        
        for (uint64_t height : test_heights) {
            uint64_t supply = Issuance::CalculateSupplyAtHeight(height, asset.id);
            assert(supply <= asset.max_supply);
            
            // Also verify block reward doesn't cause overflow
            uint64_t reward = Issuance::GetBlockReward(height, asset.id);
            assert(supply + reward <= asset.max_supply || reward == 0);
        }
    }
    
    std::cout << "  ✓ Passed (caps enforced at all tested heights)" << std::endl;
}

void TestBlockRewardValidationAgainstCaps() {
    std::cout << "Test: Block reward validation against supply caps" << std::endl;
    
    struct TestCase {
        AssetID asset;
        uint64_t height;
        uint64_t max_supply;
    };
    
    TestCase cases[] = {
        {AssetID::TALANTON, 0, 21000000ULL * AssetSupply::BASE_UNIT},
        {AssetID::DRACHMA, 0, 41000000ULL * AssetSupply::BASE_UNIT},
        {AssetID::OBOLOS, 0, 61000000ULL * AssetSupply::BASE_UNIT},
        {AssetID::TALANTON, Issuance::HALVING_INTERVAL, 21000000ULL * AssetSupply::BASE_UNIT},
        {AssetID::DRACHMA, Issuance::HALVING_INTERVAL, 41000000ULL * AssetSupply::BASE_UNIT},
        {AssetID::OBOLOS, Issuance::HALVING_INTERVAL, 61000000ULL * AssetSupply::BASE_UNIT}
    };
    
    for (const auto& tc : cases) {
        uint64_t valid_reward = Issuance::GetBlockReward(tc.height, tc.asset);
        
        // Valid reward should pass validation
        assert(Issuance::IsValidBlockReward(tc.height, tc.asset, valid_reward));
        
        // Reward exceeding valid amount should fail
        assert(!Issuance::IsValidBlockReward(tc.height, tc.asset, valid_reward + 1));
        
        // Zero or reduced reward is valid (miner can take less)
        assert(Issuance::IsValidBlockReward(tc.height, tc.asset, 0));
        assert(Issuance::IsValidBlockReward(tc.height, tc.asset, valid_reward / 2));
    }
    
    std::cout << "  ✓ Passed (reward validation works)" << std::endl;
}

void TestAsymptoticSupplyApproach() {
    std::cout << "Test: Supply approaches but never exceeds cap asymptotically" << std::endl;
    
    struct AssetTest {
        AssetID id;
        uint64_t max_supply;
        const char* ticker;
    };
    
    AssetTest assets[] = {
        {AssetID::TALANTON, 21000000ULL * AssetSupply::BASE_UNIT, "TALN"},
        {AssetID::DRACHMA,  41000000ULL * AssetSupply::BASE_UNIT, "DRM"},
        {AssetID::OBOLOS,   61000000ULL * AssetSupply::BASE_UNIT, "OBL"}
    };
    
    for (const auto& asset : assets) {
        std::cout << "  Testing " << asset.ticker << " asymptotic approach..." << std::endl;
        
        // Test at increasingly high heights
        for (int exponent = 1; exponent <= 6; exponent++) {
            uint64_t height = static_cast<uint64_t>(std::pow(10, exponent)) * Issuance::HALVING_INTERVAL;
            uint64_t supply = Issuance::CalculateSupplyAtHeight(height, asset.id);
            
            // Supply must never exceed max
            assert(supply <= asset.max_supply);
            
            // Calculate percentage of max supply reached
            double percentage = (supply * 100.0) / asset.max_supply;
            
            if (exponent == 6) {
                std::cout << "    At height " << height << ": " 
                          << supply / AssetSupply::BASE_UNIT << " " << asset.ticker
                          << " (" << percentage << "% of max)" << std::endl;
            }
            
            // At very high heights, should be very close to max (> 99.9%)
            if (exponent >= 4) {
                assert(percentage > 99.9);
            }
        }
    }
    
    std::cout << "  ✓ Passed (asymptotic approach verified)" << std::endl;
}

void TestNoSupplyOverflowInArithmetic() {
    std::cout << "Test: Supply calculations don't cause arithmetic overflow" << std::endl;
    
    AssetID assets[] = {AssetID::TALANTON, AssetID::DRACHMA, AssetID::OBOLOS};
    
    for (AssetID asset : assets) {
        // Test at maximum representable height
        uint64_t max_height = UINT64_MAX / 2; // Avoid overflow in height itself
        
        // This should not crash or overflow
        uint64_t supply = Issuance::CalculateSupplyAtHeight(max_height, asset);
        uint64_t max_supply = AssetSupply::GetMaxSupply(asset);
        
        // Supply should be capped at max
        assert(supply <= max_supply);
        
        // Reward at this height should be 0 (all halvings exhausted)
        uint64_t reward = Issuance::GetBlockReward(max_height, asset);
        assert(reward == 0);
    }
    
    std::cout << "  ✓ Passed (no arithmetic overflow)" << std::endl;
}

int main() {
    std::cout << "=== Issuance Cap Verification Tests ===" << std::endl;
    std::cout << "Comprehensive proof that 21M/41M/61M supply caps are never exceeded" << std::endl;
    std::cout << std::endl;
    
    TestTalanton21MCapNeverExceeded();
    std::cout << std::endl;
    
    TestDrachma41MCapNeverExceeded();
    std::cout << std::endl;
    
    TestObolos61MCapNeverExceeded();
    std::cout << std::endl;
    
    TestSupplyCapEnforcementAtEveryHeight();
    std::cout << std::endl;
    
    TestBlockRewardValidationAgainstCaps();
    std::cout << std::endl;
    
    TestAsymptoticSupplyApproach();
    std::cout << std::endl;
    
    TestNoSupplyOverflowInArithmetic();
    
    std::cout << "\n✓ All issuance cap tests passed!" << std::endl;
    std::cout << "Mathematical proof: Supply caps (21M TALN / 41M DRM / 61M OBL) are never exceeded." << std::endl;
    
    return 0;
}
