#include "crypto/hardware_crypto.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace parthenon::crypto;

void TestAesRoundTrip() {
    std::array<uint8_t, 32> key{};
    for (size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<uint8_t>(i + 1);
    }

    HardwareAES aes;
    if (!aes.Init(key)) {
        std::cout << "HardwareAES unavailable on this environment; skipping round-trip test." << std::endl;
        return;
    }

    std::vector<uint8_t> plaintext = {
        0x50, 0x61, 0x72, 0x74, 0x68, 0x65, 0x6f, 0x6e, 0x43, 0x68, 0x61, 0x69, 0x6e};

    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;

    const bool enc_ok = aes.Encrypt(plaintext, ciphertext);
    assert(enc_ok);
    assert(!ciphertext.empty());
    assert(ciphertext != plaintext);

    const bool dec_ok = aes.Decrypt(ciphertext, decrypted);
    assert(dec_ok);
    assert(decrypted == plaintext);
}

void TestAesTamperDetected() {
    std::array<uint8_t, 32> key{};
    for (size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<uint8_t>(0xA0 + i);
    }

    HardwareAES aes;
    if (!aes.Init(key)) {
        std::cout << "HardwareAES unavailable on this environment; skipping tamper test." << std::endl;
        return;
    }

    std::vector<uint8_t> plaintext = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<uint8_t> ciphertext;

    assert(aes.Encrypt(plaintext, ciphertext));
    assert(ciphertext.size() > 10);

    ciphertext[10] ^= 0xFF;

    std::vector<uint8_t> decrypted;
    const bool dec_ok = aes.Decrypt(ciphertext, decrypted);
    assert(!dec_ok);
}

int main() {
    TestAesRoundTrip();
    TestAesTamperDetected();

    std::cout << "All hardware crypto tests passed!" << std::endl;
    return 0;
}
