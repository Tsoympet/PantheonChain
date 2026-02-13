// ParthenonChain - Hardware Wallet Unit Tests

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include "wallet/hardware/firmware_verification.h"
#include "wallet/hardware/hardware_wallet.h"

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

void test_firmware_version_tracking() {
    FirmwareVerifier verifier;

    VendorKeys keys;
    keys.vendor_name = "VersionVendor";
    keys.public_keys.push_back(std::vector<uint8_t>(32, 0x11));
    verifier.AddVendorKeys(keys);

    FirmwareInfo v1;
    v1.vendor = "VersionVendor";
    v1.version = "1.0.0";
    v1.hash = std::vector<uint8_t>(32, 0xA1);
    verifier.AddKnownFirmware(v1);

    FirmwareInfo v2;
    v2.vendor = "VersionVendor";
    v2.version = "1.2.0";
    v2.hash = std::vector<uint8_t>(32, 0xA2);
    verifier.AddKnownFirmware(v2);

    auto latest_for_old = verifier.CheckLatestVersion("VersionVendor", "1.0.5");
    assert(latest_for_old.has_value());
    assert(*latest_for_old == "1.2.0");

    auto latest_for_current = verifier.CheckLatestVersion("VersionVendor", "1.2.0");
    assert(!latest_for_current.has_value());

    auto not_found = verifier.CheckLatestVersion("UnknownVendor", "1.0.0");
    assert(!not_found.has_value());

    auto info = verifier.GetFirmwareInfo("VersionVendor", "1.2.0");
    assert(info.has_value());
    assert(info->hash == v2.hash);

    std::cout << "✓ Firmware version tracking tests passed\n";
}

void test_firmware_key_rotation_and_revocation() {
    using parthenon::crypto::Schnorr;
    using parthenon::crypto::SHA256;

    FirmwareVerifier verifier;

    Schnorr::PrivateKey key1{};
    key1[31] = 0x10;
    Schnorr::PrivateKey key2{};
    key2[31] = 0x20;

    auto pubkey1_opt = Schnorr::GetPublicKey(key1);
    auto pubkey2_opt = Schnorr::GetPublicKey(key2);
    assert(pubkey1_opt.has_value());
    assert(pubkey2_opt.has_value());

    VendorKeys keys;
    keys.vendor_name = "RotateVendor";
    keys.public_keys.emplace_back(pubkey1_opt->begin(), pubkey1_opt->end());
    keys.public_keys.emplace_back(pubkey2_opt->begin(), pubkey2_opt->end());
    verifier.AddVendorKeys(keys);

    std::vector<uint8_t> firmware = {0xAA, 0xBB, 0xCC, 0xDD};
    auto firmware_hash_arr = SHA256::Hash256(firmware);

    auto signature1_opt = Schnorr::Sign(key1, firmware_hash_arr.data());
    assert(signature1_opt.has_value());

    FirmwareInfo info;
    info.vendor = "RotateVendor";
    info.version = "1.0.0";
    info.hash.assign(firmware_hash_arr.begin(), firmware_hash_arr.end());
    info.signature.assign(signature1_opt->begin(), signature1_opt->end());
    verifier.AddKnownFirmware(info);

    auto result = verifier.VerifyFirmware(firmware, "RotateVendor");
    assert(result.status == VerificationStatus::VALID);

    verifier.RevokeVendorKey("RotateVendor",
                             std::vector<uint8_t>(pubkey1_opt->begin(), pubkey1_opt->end()));
    auto revoked_result = verifier.VerifyFirmware(firmware, "RotateVendor");
    assert(revoked_result.status == VerificationStatus::INVALID_SIGNATURE);

    auto signature2_opt = Schnorr::Sign(key2, firmware_hash_arr.data());
    assert(signature2_opt.has_value());
    info.signature.assign(signature2_opt->begin(), signature2_opt->end());
    verifier.AddKnownFirmware(info);

    auto rotated_result = verifier.VerifyFirmware(firmware, "RotateVendor");
    assert(rotated_result.status == VerificationStatus::VALID);

    std::cout << "✓ Firmware key rotation and revocation tests passed\n";
}

void test_firmware_security_advisory_revocation() {
    using parthenon::crypto::Schnorr;
    using parthenon::crypto::SHA256;

    FirmwareVerifier verifier;

    Schnorr::PrivateKey key{};
    key[31] = 0x33;
    auto pubkey_opt = Schnorr::GetPublicKey(key);
    assert(pubkey_opt.has_value());

    VendorKeys keys;
    keys.vendor_name = "AdvisoryVendor";
    keys.public_keys.emplace_back(pubkey_opt->begin(), pubkey_opt->end());
    verifier.AddVendorKeys(keys);

    std::vector<uint8_t> firmware = {0x10, 0x20, 0x30};
    auto firmware_hash_arr = SHA256::Hash256(firmware);
    auto signature_opt = Schnorr::Sign(key, firmware_hash_arr.data());
    assert(signature_opt.has_value());

    FirmwareInfo info;
    info.vendor = "AdvisoryVendor";
    info.version = "2.0.0";
    info.hash.assign(firmware_hash_arr.begin(), firmware_hash_arr.end());
    info.signature.assign(signature_opt->begin(), signature_opt->end());
    verifier.AddKnownFirmware(info);
    verifier.AddSecurityAdvisory("AdvisoryVendor", "2.0.0", "CVE-2026-0001");

    auto result = verifier.VerifyFirmware(firmware, "AdvisoryVendor");
    assert(result.status == VerificationStatus::EXPIRED);

    std::cout << "✓ Firmware advisory revocation tests passed\n";
}

void test_firmware_anti_rollback_checks() {
    using parthenon::crypto::Schnorr;
    using parthenon::crypto::SHA256;

    FirmwareVerifier verifier;

    Schnorr::PrivateKey key{};
    key[31] = 0x44;
    auto pubkey_opt = Schnorr::GetPublicKey(key);
    assert(pubkey_opt.has_value());

    VendorKeys keys;
    keys.vendor_name = "RollbackVendor";
    keys.public_keys.emplace_back(pubkey_opt->begin(), pubkey_opt->end());
    verifier.AddVendorKeys(keys);

    std::vector<uint8_t> firmware = {0x01, 0x02, 0x03, 0x04};
    auto firmware_hash_arr = SHA256::Hash256(firmware);
    auto signature_opt = Schnorr::Sign(key, firmware_hash_arr.data());
    assert(signature_opt.has_value());

    FirmwareInfo info;
    info.vendor = "RollbackVendor";
    info.version = "1.0.0";
    info.hash.assign(firmware_hash_arr.begin(), firmware_hash_arr.end());
    info.signature.assign(signature_opt->begin(), signature_opt->end());
    verifier.AddKnownFirmware(info);

    auto rollback_result = verifier.VerifyFirmwareUpdate(firmware, "RollbackVendor", "1.2.0");
    assert(rollback_result.status == VerificationStatus::EXPIRED);

    auto valid_result = verifier.VerifyFirmwareUpdate(firmware, "RollbackVendor", "0.9.0");
    assert(valid_result.status == VerificationStatus::VALID);

    std::cout << "✓ Firmware anti-rollback tests passed\n";
}

void test_supply_chain_stolen_registry() {
    assert(!SupplyChainVerifier::CheckStolenRegistry(""));
    assert(!SupplyChainVerifier::CheckStolenRegistry("STOLEN-0001"));
    assert(SupplyChainVerifier::CheckStolenRegistry("SAFE-0001"));

    std::cout << "✓ Supply chain stolen registry tests passed\n";
}

int main() {
    std::cout << "Running hardware wallet tests...\n\n";

    test_derivation_path_parsing();
    test_derivation_path_construction();
    test_hardware_wallet_manager();
    test_firmware_signature_verification();
    test_firmware_version_tracking();
    test_firmware_key_rotation_and_revocation();
    test_firmware_security_advisory_revocation();
    test_firmware_anti_rollback_checks();
    test_supply_chain_stolen_registry();

    std::cout << "\nAll hardware wallet tests passed!\n";
    return 0;
}
