// ParthenonChain - Schnorr Signature Test Vectors
// Deterministic tests using BIP-340 test vectors

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace parthenon::crypto;

// Helper to convert hex string to bytes
std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::strtol(byte_str.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Helper to convert bytes to hex string
std::string BytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

template <size_t N>
std::string BytesToHex(const std::array<uint8_t, N>& arr) {
    return BytesToHex(arr.data(), arr.size());
}

void TestPrivateKeyValidation() {
    std::cout << "Test: Private key validation" << std::endl;

    // Valid private key
    auto valid_key_bytes =
        HexToBytes("0000000000000000000000000000000000000000000000000000000000000001");
    Schnorr::PrivateKey valid_key;
    std::copy(valid_key_bytes.begin(), valid_key_bytes.end(), valid_key.begin());
    assert(Schnorr::ValidatePrivateKey(valid_key));

    // Invalid private key (all zeros)
    Schnorr::PrivateKey invalid_key{};
    assert(!Schnorr::ValidatePrivateKey(invalid_key));

    // Invalid private key (greater than curve order)
    auto invalid_bytes =
        HexToBytes("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
    Schnorr::PrivateKey invalid_key2;
    std::copy(invalid_bytes.begin(), invalid_bytes.end(), invalid_key2.begin());
    assert(!Schnorr::ValidatePrivateKey(invalid_key2));

    std::cout << "  ✓ Passed" << std::endl;
}

void TestPublicKeyDerivation() {
    std::cout << "Test: Public key derivation" << std::endl;

    // Test vector from BIP-340
    auto privkey_bytes =
        HexToBytes("0000000000000000000000000000000000000000000000000000000000000001");
    Schnorr::PrivateKey privkey;
    std::copy(privkey_bytes.begin(), privkey_bytes.end(), privkey.begin());

    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());

    std::string pubkey_hex = BytesToHex(*pubkey_opt);
    std::cout << "  Public key: " << pubkey_hex << std::endl;

    // Verify it's deterministic
    auto pubkey_opt2 = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt2.has_value());
    assert(*pubkey_opt == *pubkey_opt2);

    std::cout << "  ✓ Passed (deterministic)" << std::endl;
}

void TestSignAndVerify() {
    std::cout << "Test: Sign and verify" << std::endl;

    // Generate a test private key
    auto privkey_bytes =
        HexToBytes("C90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B14E5C9");
    Schnorr::PrivateKey privkey;
    std::copy(privkey_bytes.begin(), privkey_bytes.end(), privkey.begin());

    // Get public key
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());

    // Create a message hash
    const char* msg = "test message";
    auto msg_hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(msg), 12);

    // Sign the message
    auto sig_opt = Schnorr::Sign(privkey, msg_hash.data(), nullptr);
    assert(sig_opt.has_value());

    std::cout << "  Signature: " << BytesToHex(*sig_opt) << std::endl;

    // Verify the signature
    bool valid = Schnorr::Verify(*pubkey_opt, msg_hash.data(), *sig_opt);
    assert(valid);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestDeterministicSigning() {
    std::cout << "Test: Deterministic signing (same message, same signature)" << std::endl;

    auto privkey_bytes =
        HexToBytes("0000000000000000000000000000000000000000000000000000000000000003");
    Schnorr::PrivateKey privkey;
    std::copy(privkey_bytes.begin(), privkey_bytes.end(), privkey.begin());

    const char* msg = "ParthenonChain";
    auto msg_hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(msg), 14);

    // Sign twice with same parameters
    auto sig1_opt = Schnorr::Sign(privkey, msg_hash.data(), nullptr);
    auto sig2_opt = Schnorr::Sign(privkey, msg_hash.data(), nullptr);

    assert(sig1_opt.has_value());
    assert(sig2_opt.has_value());

    // Signatures should be identical (deterministic)
    assert(*sig1_opt == *sig2_opt);

    std::cout << "  ✓ Passed (deterministic)" << std::endl;
}

void TestInvalidSignature() {
    std::cout << "Test: Invalid signature rejection" << std::endl;

    auto privkey_bytes =
        HexToBytes("0000000000000000000000000000000000000000000000000000000000000002");
    Schnorr::PrivateKey privkey;
    std::copy(privkey_bytes.begin(), privkey_bytes.end(), privkey.begin());

    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());

    const char* msg = "original message";
    auto msg_hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(msg), 16);

    auto sig_opt = Schnorr::Sign(privkey, msg_hash.data(), nullptr);
    assert(sig_opt.has_value());

    // Verify with correct message
    bool valid1 = Schnorr::Verify(*pubkey_opt, msg_hash.data(), *sig_opt);
    assert(valid1);

    // Verify with different message (should fail)
    const char* wrong_msg = "modified message";
    auto wrong_hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(wrong_msg), 16);
    bool valid2 = Schnorr::Verify(*pubkey_opt, wrong_hash.data(), *sig_opt);
    assert(!valid2);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestAuxiliaryRandomness() {
    std::cout << "Test: Signing with auxiliary randomness" << std::endl;

    auto privkey_bytes =
        HexToBytes("0000000000000000000000000000000000000000000000000000000000000005");
    Schnorr::PrivateKey privkey;
    std::copy(privkey_bytes.begin(), privkey_bytes.end(), privkey.begin());

    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());

    const char* msg = "test";
    auto msg_hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(msg), 4);

    // Create auxiliary randomness
    uint8_t aux_rand[32];
    for (int i = 0; i < 32; i++) {
        aux_rand[i] = static_cast<uint8_t>(i);
    }

    auto sig_opt = Schnorr::Sign(privkey, msg_hash.data(), aux_rand);
    assert(sig_opt.has_value() && Schnorr::Verify(*pubkey_opt, msg_hash.data(), *sig_opt));

    std::cout << "  ✓ Passed" << std::endl;
}

void TestBatchSignatures() {
    std::cout << "Test: Multiple signatures with same key" << std::endl;

    auto privkey_bytes =
        HexToBytes("B7E151628AED2A6ABF7158809CF4F3C762E7160F38B4DA56A784D9045190CFEF");
    Schnorr::PrivateKey privkey;
    std::copy(privkey_bytes.begin(), privkey_bytes.end(), privkey.begin());

    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());

    // Sign multiple different messages
    std::vector<std::string> messages = {"msg1", "msg2", "msg3", "msg4", "msg5"};

    for (const auto& msg : messages) {
        auto msg_hash =
            SHA256::Hash256(reinterpret_cast<const uint8_t*>(msg.c_str()), msg.length());

        auto sig_opt = Schnorr::Sign(privkey, msg_hash.data(), nullptr);
        assert(sig_opt.has_value() && Schnorr::Verify(*pubkey_opt, msg_hash.data(), *sig_opt));
    }

    std::cout << "  ✓ Passed (5 signatures)" << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "ParthenonChain Schnorr Test Suite" << std::endl;
    std::cout << "BIP-340 Implementation" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;

    try {
        TestPrivateKeyValidation();
        TestPublicKeyDerivation();
        TestSignAndVerify();
        TestDeterministicSigning();
        TestInvalidSignature();
        TestAuxiliaryRandomness();
        TestBatchSignatures();

        std::cout << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "All Schnorr tests passed! ✓" << std::endl;
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
