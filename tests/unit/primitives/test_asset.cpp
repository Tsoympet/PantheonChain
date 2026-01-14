// ParthenonChain - Asset Tests
// Test asset IDs, supply caps, and asset amounts

#include "primitives/asset.h"

#include <cassert>
#include <iostream>

using namespace parthenon::primitives;

void TestAssetSupplyCaps() {
    std::cout << "Test: Asset supply caps" << std::endl;

    // Verify supply constants
    assert(AssetSupply::TALN_MAX_SUPPLY == 2100000000000000ULL);
    assert(AssetSupply::DRM_MAX_SUPPLY == 4100000000000000ULL);
    assert(AssetSupply::OBL_MAX_SUPPLY == 6100000000000000ULL);

    // Verify base unit
    assert(AssetSupply::BASE_UNIT == 100000000ULL);

    // Verify GetMaxSupply
    assert(AssetSupply::GetMaxSupply(AssetID::TALANTON) == AssetSupply::TALN_MAX_SUPPLY);
    assert(AssetSupply::GetMaxSupply(AssetID::DRACHMA) == AssetSupply::DRM_MAX_SUPPLY);
    assert(AssetSupply::GetMaxSupply(AssetID::OBOLOS) == AssetSupply::OBL_MAX_SUPPLY);

    std::cout << "  TALN max: " << AssetSupply::TALN_MAX_SUPPLY << std::endl;
    std::cout << "  DRM max:  " << AssetSupply::DRM_MAX_SUPPLY << std::endl;
    std::cout << "  OBL max:  " << AssetSupply::OBL_MAX_SUPPLY << std::endl;
    std::cout << "  ✓ Passed" << std::endl;
}

void TestAssetValidation() {
    std::cout << "Test: Asset amount validation" << std::endl;

    // Valid amounts
    assert(AssetSupply::IsValidAmount(AssetID::TALANTON, 0));
    assert(AssetSupply::IsValidAmount(AssetID::TALANTON, 1000000));
    assert(AssetSupply::IsValidAmount(AssetID::TALANTON, AssetSupply::TALN_MAX_SUPPLY));

    assert(AssetSupply::IsValidAmount(AssetID::DRACHMA, 0));
    assert(AssetSupply::IsValidAmount(AssetID::DRACHMA, AssetSupply::DRM_MAX_SUPPLY));

    assert(AssetSupply::IsValidAmount(AssetID::OBOLOS, 0));
    assert(AssetSupply::IsValidAmount(AssetID::OBOLOS, AssetSupply::OBL_MAX_SUPPLY));

    // Invalid amounts (exceeding max supply)
    assert(!AssetSupply::IsValidAmount(AssetID::TALANTON, AssetSupply::TALN_MAX_SUPPLY + 1));
    assert(!AssetSupply::IsValidAmount(AssetID::DRACHMA, AssetSupply::DRM_MAX_SUPPLY + 1));
    assert(!AssetSupply::IsValidAmount(AssetID::OBOLOS, AssetSupply::OBL_MAX_SUPPLY + 1));

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAssetNames() {
    std::cout << "Test: Asset names and tickers" << std::endl;

    // Names
    assert(AssetSupply::GetAssetName(AssetID::TALANTON) == "TALANTON");
    assert(AssetSupply::GetAssetName(AssetID::DRACHMA) == "DRACHMA");
    assert(AssetSupply::GetAssetName(AssetID::OBOLOS) == "OBOLOS");

    // Tickers
    assert(AssetSupply::GetAssetTicker(AssetID::TALANTON) == "TALN");
    assert(AssetSupply::GetAssetTicker(AssetID::DRACHMA) == "DRM");
    assert(AssetSupply::GetAssetTicker(AssetID::OBOLOS) == "OBL");

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAssetAmount() {
    std::cout << "Test: AssetAmount structure" << std::endl;

    // Default constructor
    AssetAmount default_amt;
    assert(default_amt.asset == AssetID::TALANTON);
    assert(default_amt.amount == 0);

    // Constructor with values
    AssetAmount taln(AssetID::TALANTON, 1000000);
    assert(taln.asset == AssetID::TALANTON);
    assert(taln.amount == 1000000);

    AssetAmount drm(AssetID::DRACHMA, 5000000);
    assert(drm.asset == AssetID::DRACHMA);
    assert(drm.amount == 5000000);

    // Equality
    AssetAmount taln2(AssetID::TALANTON, 1000000);
    assert(taln == taln2);
    assert(taln != drm);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAssetAmountValidation() {
    std::cout << "Test: AssetAmount validation" << std::endl;

    // Valid asset amounts
    AssetAmount valid_taln(AssetID::TALANTON, 1000000);
    assert(valid_taln.IsValid());

    AssetAmount valid_drm(AssetID::DRACHMA, AssetSupply::DRM_MAX_SUPPLY);
    assert(valid_drm.IsValid());

    AssetAmount zero_obl(AssetID::OBOLOS, 0);
    assert(zero_obl.IsValid());

    // Invalid asset amounts (exceeding max supply)
    AssetAmount invalid_taln(AssetID::TALANTON, AssetSupply::TALN_MAX_SUPPLY + 1);
    assert(!invalid_taln.IsValid());

    AssetAmount invalid_drm(AssetID::DRACHMA, AssetSupply::DRM_MAX_SUPPLY + 1);
    assert(!invalid_drm.IsValid());

    AssetAmount invalid_obl(AssetID::OBOLOS, AssetSupply::OBL_MAX_SUPPLY + 1);
    assert(!invalid_obl.IsValid());

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAssetAmountSerialization() {
    std::cout << "Test: AssetAmount serialization" << std::endl;

    uint8_t buffer[9];  // 1 byte asset + 8 bytes amount

    // Test TALANTON
    AssetAmount taln(AssetID::TALANTON, 123456789);
    taln.Serialize(buffer);
    AssetAmount taln_deser = AssetAmount::Deserialize(buffer);
    assert(taln == taln_deser);
    assert(buffer[0] == 0);  // TALANTON = 0

    // Test DRACHMA
    AssetAmount drm(AssetID::DRACHMA, 987654321);
    drm.Serialize(buffer);
    AssetAmount drm_deser = AssetAmount::Deserialize(buffer);
    assert(drm == drm_deser);
    assert(buffer[0] == 1);  // DRACHMA = 1

    // Test OBOLOS
    AssetAmount obl(AssetID::OBOLOS, 555555555);
    obl.Serialize(buffer);
    AssetAmount obl_deser = AssetAmount::Deserialize(buffer);
    assert(obl == obl_deser);
    assert(buffer[0] == 2);  // OBOLOS = 2

    // Test max amounts
    AssetAmount max_taln(AssetID::TALANTON, AssetSupply::TALN_MAX_SUPPLY);
    max_taln.Serialize(buffer);
    AssetAmount max_taln_deser = AssetAmount::Deserialize(buffer);
    assert(max_taln == max_taln_deser);

    // Test zero
    AssetAmount zero(AssetID::DRACHMA, 0);
    zero.Serialize(buffer);
    AssetAmount zero_deser = AssetAmount::Deserialize(buffer);
    assert(zero == zero_deser);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAssetConservation() {
    std::cout << "Test: Asset conservation (no mixing)" << std::endl;

    // Each asset type is separate
    AssetAmount taln1(AssetID::TALANTON, 1000);
    AssetAmount taln2(AssetID::TALANTON, 2000);
    AssetAmount drm1(AssetID::DRACHMA, 1000);

    // Same asset, same amount should be equal
    AssetAmount taln1_copy(AssetID::TALANTON, 1000);
    assert(taln1 == taln1_copy);

    // Same amount, different asset should NOT be equal
    assert(taln1 != drm1);

    // Different amounts of same asset should NOT be equal
    assert(taln1 != taln2);

    std::cout << "  ✓ Passed (assets cannot be mixed)" << std::endl;
}

void TestSupplyEnforcement() {
    std::cout << "Test: Supply cap enforcement" << std::endl;

    // Verify that supply caps are enforced
    // This ensures no asset can exceed its maximum supply

    // Test at boundary
    AssetAmount at_max_taln(AssetID::TALANTON, AssetSupply::TALN_MAX_SUPPLY);
    assert(at_max_taln.IsValid());

    AssetAmount at_max_drm(AssetID::DRACHMA, AssetSupply::DRM_MAX_SUPPLY);
    assert(at_max_drm.IsValid());

    AssetAmount at_max_obl(AssetID::OBOLOS, AssetSupply::OBL_MAX_SUPPLY);
    assert(at_max_obl.IsValid());

    // Test one over boundary
    AssetAmount over_max_taln(AssetID::TALANTON, AssetSupply::TALN_MAX_SUPPLY + 1);
    assert(!over_max_taln.IsValid());

    AssetAmount over_max_drm(AssetID::DRACHMA, AssetSupply::DRM_MAX_SUPPLY + 1);
    assert(!over_max_drm.IsValid());

    AssetAmount over_max_obl(AssetID::OBOLOS, AssetSupply::OBL_MAX_SUPPLY + 1);
    assert(!over_max_obl.IsValid());

    std::cout << "  ✓ Passed (supply caps enforced)" << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "ParthenonChain Asset Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;

    try {
        TestAssetSupplyCaps();
        TestAssetValidation();
        TestAssetNames();
        TestAssetAmount();
        TestAssetAmountValidation();
        TestAssetAmountSerialization();
        TestAssetConservation();
        TestSupplyEnforcement();

        std::cout << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "All Asset tests passed! ✓" << std::endl;
        std::cout << "=====================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
