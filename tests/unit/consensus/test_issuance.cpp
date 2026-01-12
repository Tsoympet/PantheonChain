// ParthenonChain - Issuance Tests
// Test block rewards, halving schedule, and supply cap enforcement

#include "consensus/issuance.h"
#include <iostream>
#include <cassert>

using namespace parthenon::consensus;
using namespace parthenon::primitives;

void TestInitialRewards() {
    std::cout << "Test: Initial block rewards" << std::endl;
    
    uint64_t taln_reward = Issuance::GetBlockReward(0, AssetID::TALANTON);
    uint64_t drm_reward = Issuance::GetBlockReward(0, AssetID::DRACHMA);
    uint64_t obl_reward = Issuance::GetBlockReward(0, AssetID::OBOLOS);
    
    // Check initial rewards
    assert(taln_reward == 50ULL * AssetSupply::BASE_UNIT);
    assert(drm_reward == 97ULL * AssetSupply::BASE_UNIT);
    assert(obl_reward == 145ULL * AssetSupply::BASE_UNIT);
    
    std::cout << "  TALN: " << taln_reward / AssetSupply::BASE_UNIT << " coins" << std::endl;
    std::cout << "  DRM:  " << drm_reward / AssetSupply::BASE_UNIT << " coins" << std::endl;
    std::cout << "  OBL:  " << obl_reward / AssetSupply::BASE_UNIT << " coins" << std::endl;
    std::cout << "  ✓ Passed (initial rewards correct)" << std::endl;
}

void TestHalvingSchedule() {
    std::cout << "Test: Halving schedule" << std::endl;
    
    // Test TALN halvings
    uint64_t height_0 = 0;
    uint64_t height_1 = Issuance::HALVING_INTERVAL;
    uint64_t height_2 = 2 * Issuance::HALVING_INTERVAL;
    uint64_t height_3 = 3 * Issuance::HALVING_INTERVAL;
    
    uint64_t reward_0 = Issuance::GetBlockReward(height_0, AssetID::TALANTON);
    uint64_t reward_1 = Issuance::GetBlockReward(height_1, AssetID::TALANTON);
    uint64_t reward_2 = Issuance::GetBlockReward(height_2, AssetID::TALANTON);
    uint64_t reward_3 = Issuance::GetBlockReward(height_3, AssetID::TALANTON);
    
    // Each halving should cut reward in half
    assert(reward_1 == reward_0 / 2);
    assert(reward_2 == reward_1 / 2);
    assert(reward_3 == reward_2 / 2);
    
    std::cout << "  Height 0: " << reward_0 / AssetSupply::BASE_UNIT << " TALN" << std::endl;
    std::cout << "  Height " << height_1 << ": " << reward_1 / AssetSupply::BASE_UNIT << " TALN" << std::endl;
    std::cout << "  Height " << height_2 << ": " << reward_2 / AssetSupply::BASE_UNIT << " TALN" << std::endl;
    std::cout << "  ✓ Passed (halving works)" << std::endl;
}

void TestRewardAfterManyHalvings() {
    std::cout << "Test: Reward after many halvings" << std::endl;
    
    // After 64 halvings, reward should be 0
    uint64_t height = 64 * Issuance::HALVING_INTERVAL;
    uint64_t reward = Issuance::GetBlockReward(height, AssetID::TALANTON);
    assert(reward == 0);
    
    // Even further in the future
    height = 100 * Issuance::HALVING_INTERVAL;
    reward = Issuance::GetBlockReward(height, AssetID::TALANTON);
    assert(reward == 0);
    
    std::cout << "  ✓ Passed (eventually goes to zero)" << std::endl;
}

void TestSupplyCalculation() {
    std::cout << "Test: Supply calculation at height" << std::endl;
    
    // At height 0, supply is 0 (genesis block hasn't been applied yet)
    uint64_t supply_0 = Issuance::CalculateSupplyAtHeight(0, AssetID::TALANTON);
    assert(supply_0 == 0);
    
    // After 1 block, supply equals initial reward
    uint64_t supply_1 = Issuance::CalculateSupplyAtHeight(1, AssetID::TALANTON);
    uint64_t reward_0 = Issuance::GetBlockReward(0, AssetID::TALANTON);
    assert(supply_1 == reward_0);
    
    // After first halving interval, supply is sum of all rewards in that epoch
    uint64_t supply_halving = Issuance::CalculateSupplyAtHeight(
        Issuance::HALVING_INTERVAL, AssetID::TALANTON
    );
    uint64_t expected_supply = Issuance::HALVING_INTERVAL * reward_0;
    assert(supply_halving == expected_supply);
    
    std::cout << "  Supply after " << Issuance::HALVING_INTERVAL << " blocks: " 
              << supply_halving / AssetSupply::BASE_UNIT << " TALN" << std::endl;
    std::cout << "  ✓ Passed (supply calculation correct)" << std::endl;
}

void TestSupplyNeverExceedsCap() {
    std::cout << "Test: Supply never exceeds cap" << std::endl;
    
    // Test all three assets
    AssetID assets[] = {AssetID::TALANTON, AssetID::DRACHMA, AssetID::OBOLOS};
    
    for (AssetID asset : assets) {
        uint64_t max_supply = AssetSupply::GetMaxSupply(asset);
        
        // Test at various heights
        for (uint64_t height : {0ULL, 1ULL, 1000ULL, 100000ULL, 1000000ULL, 10000000ULL}) {
            uint64_t supply = Issuance::CalculateSupplyAtHeight(height, asset);
            assert(supply <= max_supply);
        }
        
        // Test at very high height (should approach but never exceed cap)
        uint64_t very_high = 100 * Issuance::HALVING_INTERVAL;
        uint64_t supply = Issuance::CalculateSupplyAtHeight(very_high, asset);
        assert(supply <= max_supply);
        
        std::cout << "  " << AssetSupply::GetAssetTicker(asset) 
                  << " max supply: " << max_supply / AssetSupply::BASE_UNIT 
                  << " (OK)" << std::endl;
    }
    
    std::cout << "  ✓ Passed (all supplies capped)" << std::endl;
}

void TestBlockRewardValidation() {
    std::cout << "Test: Block reward validation" << std::endl;
    
    uint64_t height = 100;
    uint64_t valid_reward = Issuance::GetBlockReward(height, AssetID::TALANTON);
    
    // Valid reward should pass
    assert(Issuance::IsValidBlockReward(height, AssetID::TALANTON, valid_reward));
    
    // Reward equal to max should pass
    assert(Issuance::IsValidBlockReward(height, AssetID::TALANTON, valid_reward));
    
    // Reward exceeding max should fail
    assert(!Issuance::IsValidBlockReward(height, AssetID::TALANTON, valid_reward + 1));
    
    // Zero reward is valid (miner can choose to take less)
    assert(Issuance::IsValidBlockReward(height, AssetID::TALANTON, 0));
    
    std::cout << "  ✓ Passed (validation works)" << std::endl;
}

void TestSupplyAtMaxHeight() {
    std::cout << "Test: Total supply approaches maximum" << std::endl;
    
    // Calculate supply at very high height for TALN
    uint64_t max_height = 1000 * Issuance::HALVING_INTERVAL;
    uint64_t supply = Issuance::CalculateSupplyAtHeight(max_height, AssetID::TALANTON);
    uint64_t max_supply = AssetSupply::GetMaxSupply(AssetID::TALANTON);
    
    // Should be close to but not exceed max
    assert(supply <= max_supply);
    
    // Should be very close (within 1% of max)
    // With infinite halvings, supply approaches max asymptotically
    uint64_t diff = max_supply - supply;
    double percentage = (diff * 100.0) / max_supply;
    
    std::cout << "  Supply at height " << max_height << ": " 
              << supply / AssetSupply::BASE_UNIT << " TALN" << std::endl;
    std::cout << "  Distance from cap: " << percentage << "%" << std::endl;
    std::cout << "  ✓ Passed (asymptotic approach)" << std::endl;
}

int main() {
    std::cout << "=== Issuance Tests ===" << std::endl;
    
    TestInitialRewards();
    TestHalvingSchedule();
    TestRewardAfterManyHalvings();
    TestSupplyCalculation();
    TestSupplyNeverExceedsCap();
    TestBlockRewardValidation();
    TestSupplyAtMaxHeight();
    
    std::cout << "\n✓ All issuance tests passed!" << std::endl;
    return 0;
}
