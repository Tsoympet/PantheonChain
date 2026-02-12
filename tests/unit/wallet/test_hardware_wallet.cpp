// ParthenonChain - Hardware Wallet Unit Tests

#include "wallet/hardware/hardware_wallet.h"
#include "wallet/hardware/firmware_verification.h"
#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <cassert>
#include <iostream>

using namespace parthenon::wallet::hardware;

void test_derivation_path_parsing() {
    // Test valid paths
    auto path1 = DerivationPath::Parse("m/44'/0'/0'/0/0");
    assert(path1.has_value());
    assert(path1->path.size() == 5);
    assert((path1->path[0] & 0x80000000) != 0);  // Hardened
    assert((path1->path[1] & 0x80000000) != 0);  // Hardened
    assert((path1->path[2] & 0x80000000) != 0);  // Hardened
    assert((path1->path[3] & 0x80000000) == 0);  // Not hardened
    assert((path1->path[4] & 0x80000000) == 0);  // Not hardened

    // Test path to string conversion
    std::string path_str = path1->ToString();
    assert(path_str == "m/44'/0'/0'/0/0");

    // Test invalid paths
    auto invalid1 = DerivationPath::Parse("invalid");
    assert(!invalid1.has_value());

    auto invalid2 = DerivationPath::Parse("");
    assert(!invalid2.has_value());

    std::cout << "✓ Derivation path parsing tests passed\n";
}

void test_hardware_wallet_manager() {
    HardwareWalletManager manager;

    // Enumerate devices (will be empty in test environment)
    auto devices = manager.EnumerateDevices();

    // Try to connect (will fail if no devices)
    auto wallet = manager.ConnectFirstDevice();

    // In test environment with no physical devices, this is expected
    if (!wallet) {
        std::cout << "✓ Hardware wallet manager created (no devices detected)\n";
    } else {
        std::cout << "✓ Hardware wallet manager created and connected to device\n";
    }
}

void test_derivation_path_construction() {
    // Test constructor with vector
    std::vector<uint32_t> path_data = {
        44 | 0x80000000,  // 44'
        0 | 0x80000000,   // 0'
        0 | 0x80000000,   // 0'
        0,                // 0
        0                 // 0
    };

    DerivationPath path(path_data);
    assert(path.path.size() == 5);
    assert(path.ToString() == "m/44'/0'/0'/0/0");

    std::cout << "✓ Derivation path construction tests passed\n";
}


void test_firmware_signature_verification() {
    using parthenon::crypto::Schnorr;
    using parthenon::crypto::SHA256;

    FirmwareVerifier verifier;

    Schnorr::PrivateKey privkey{};
    privkey[31] = 1;  // Deterministic valid key for tests

    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());

    std::vector<uint8_t> firmware = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
    auto firmware_hash_arr = SHA256::Hash256(firmware);

    auto signature_opt = Schnorr::Sign(privkey, firmware_hash_arr.data());
    assert(signature_opt.has_value());

    VendorKeys keys;
    keys.vendor_name = "UnitTestVendor";
    keys.public_keys.emplace_back(pubkey_opt->begin(), pubkey_opt->end());
    verifier.AddVendorKeys(keys);

    FirmwareInfo info;
    info.vendor = "UnitTestVendor";
    info.version = "1.0.0";
    info.hash.assign(firmware_hash_arr.begin(), firmware_hash_arr.end());
    info.signature.assign(signature_opt->begin(), signature_opt->end());
    verifier.AddKnownFirmware(info);

    auto result = verifier.VerifyFirmware(firmware, "UnitTestVendor");
    assert(result.status == VerificationStatus::VALID);

    // Tampered signature should be rejected
    auto bad_sig = info.signature;
    bad_sig[0] ^= 0x01;
    assert(!verifier.VerifySignature(firmware, bad_sig, "UnitTestVendor"));

    std::cout << "✓ Firmware Schnorr signature verification tests passed\n";
}

int main() {
    std::cout << "Running hardware wallet tests...\n\n";

    test_derivation_path_parsing();
    test_derivation_path_construction();
    test_hardware_wallet_manager();
    test_firmware_signature_verification();

    std::cout << "\nAll hardware wallet tests passed!\n";
    return 0;
}
