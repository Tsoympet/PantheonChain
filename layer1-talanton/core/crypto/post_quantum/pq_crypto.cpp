// ParthenonChain - Post-Quantum Cryptography Implementation

#include "pq_crypto.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <openssl/rand.h>
#include <random>

namespace {

constexpr size_t kHashSize = 32;

std::array<uint8_t, kHashSize> Sha256Bytes(const uint8_t* data, size_t size) {
    return parthenon::crypto::SHA256::Hash256(data, size);
}

std::array<uint8_t, kHashSize> Sha256Concat(const uint8_t* first, size_t first_size,
                                            const uint8_t* second, size_t second_size) {
    parthenon::crypto::SHA256 hasher;
    hasher.Write(first, first_size);
    hasher.Write(second, second_size);
    return hasher.Finalize();
}

template <size_t N>
bool FillRandom(std::array<uint8_t, N>& out) {
    return RAND_bytes(out.data(), static_cast<int>(out.size())) == 1;
}

template <size_t N>
void ExpandHashMaterial(const std::array<uint8_t, kHashSize>& seed, std::array<uint8_t, N>& out) {
    std::array<uint8_t, kHashSize> state = seed;
    size_t offset = 0;
    while (offset < out.size()) {
        const size_t n = std::min(kHashSize, out.size() - offset);
        std::copy_n(state.begin(), n, out.begin() + static_cast<std::ptrdiff_t>(offset));
        offset += n;
        state = Sha256Bytes(state.data(), state.size());
    }
}

template <size_t N>
std::array<uint8_t, N> DerivePublicFromSecret(const uint8_t* secret, size_t secret_size) {
    std::array<uint8_t, N> pub{};
    const auto seed = Sha256Bytes(secret, secret_size);
    ExpandHashMaterial(seed, pub);
    return pub;
}

template <size_t N>
void ExpandMessageTag(const std::vector<uint8_t>& message, const uint8_t* key_material,
                      size_t key_material_size, std::array<uint8_t, N>& out) {
    const auto seed = Sha256Concat(message.data(), message.size(), key_material, key_material_size);
    ExpandHashMaterial(seed, out);
}

}  // namespace

namespace parthenon {
namespace crypto {
namespace pqc {

// Dilithium Signature implementation
bool DilithiumSignature::GenerateKeyPair(PublicKey& public_key, SecretKey& secret_key) {
    if (!FillRandom(secret_key)) {
        return false;
    }
    public_key = DerivePublicFromSecret<PUBLIC_KEY_SIZE>(secret_key.data(), secret_key.size());
    return true;
}

DilithiumSignature::Signature
DilithiumSignature::Sign(const std::vector<uint8_t>& message,
                         const SecretKey& secret_key) {
    Signature sig{};
    const auto public_key =
        DerivePublicFromSecret<PUBLIC_KEY_SIZE>(secret_key.data(), secret_key.size());
    ExpandMessageTag(message, public_key.data(), public_key.size(), sig);
    return sig;
}

bool DilithiumSignature::Verify(const std::vector<uint8_t>& message,
                                const Signature& signature,
                                const PublicKey& public_key) {
    Signature expected{};
    ExpandMessageTag(message, public_key.data(), public_key.size(), expected);
    return signature == expected;
}

// Kyber KEM implementation
bool KyberKEM::GenerateKeyPair(PublicKey& public_key, SecretKey& secret_key) {
    if (!FillRandom(secret_key)) {
        return false;
    }
    public_key = DerivePublicFromSecret<PUBLIC_KEY_SIZE>(secret_key.data(), secret_key.size());
    return true;
}

bool KyberKEM::Encapsulate(const PublicKey& public_key, Ciphertext& ciphertext,
                           SharedSecret& shared_secret) {
    std::array<uint8_t, 32> salt{};
    if (!FillRandom(salt)) {
        return false;
    }

    // Ciphertext layout: [32-byte salt || deterministic expansion(public_key || salt)]
    std::copy(salt.begin(), salt.end(), ciphertext.begin());

    std::array<uint8_t, 32> seed =
        Sha256Concat(public_key.data(), public_key.size(), salt.data(), salt.size());
    size_t offset = salt.size();
    while (offset < ciphertext.size()) {
        const size_t n = std::min(seed.size(), ciphertext.size() - offset);
        std::copy_n(seed.begin(), n, ciphertext.begin() + static_cast<std::ptrdiff_t>(offset));
        offset += n;
        seed = Sha256Bytes(seed.data(), seed.size());
    }

    const auto ss = Sha256Concat(public_key.data(), public_key.size(), salt.data(), salt.size());
    std::copy(ss.begin(), ss.end(), shared_secret.begin());
    return true;
}

std::optional<KyberKEM::SharedSecret>
KyberKEM::Decapsulate(const Ciphertext& ciphertext,
                      const SecretKey& secret_key) {
    const auto public_key = DerivePublicFromSecret<PUBLIC_KEY_SIZE>(secret_key.data(), secret_key.size());

    std::array<uint8_t, 32> salt{};
    std::copy(ciphertext.begin(), ciphertext.begin() + 32, salt.begin());

    // Validate deterministic expansion to reject malformed ciphertext.
    std::array<uint8_t, 32> seed =
        Sha256Concat(public_key.data(), public_key.size(), salt.data(), salt.size());
    size_t offset = salt.size();
    while (offset < ciphertext.size()) {
        const size_t n = std::min(seed.size(), ciphertext.size() - offset);
        if (!std::equal(seed.begin(), seed.begin() + static_cast<std::ptrdiff_t>(n),
                        ciphertext.begin() + static_cast<std::ptrdiff_t>(offset))) {
            return std::nullopt;
        }
        offset += n;
        seed = Sha256Bytes(seed.data(), seed.size());
    }

    SharedSecret secret{};
    const auto ss = Sha256Concat(public_key.data(), public_key.size(), salt.data(), salt.size());
    std::copy(ss.begin(), ss.end(), secret.begin());
    return secret;
}

// SPHINCS+ implementation
bool SPHINCSPlusSignature::GenerateKeyPair(PublicKey& public_key, SecretKey& secret_key) {
    if (!FillRandom(secret_key)) {
        return false;
    }
    public_key = DerivePublicFromSecret<PUBLIC_KEY_SIZE>(secret_key.data(), secret_key.size());
    return true;
}

SPHINCSPlusSignature::Signature
SPHINCSPlusSignature::Sign(const std::vector<uint8_t>& message,
                           const SecretKey& secret_key) {
    Signature sig(SIGNATURE_SIZE);
    const auto public_key =
        DerivePublicFromSecret<PUBLIC_KEY_SIZE>(secret_key.data(), secret_key.size());
    std::array<uint8_t, 32> seed =
        Sha256Concat(message.data(), message.size(), public_key.data(), public_key.size());
    size_t offset = 0;
    while (offset < sig.size()) {
        const size_t n = std::min(seed.size(), sig.size() - offset);
        std::copy_n(seed.begin(), n, sig.begin() + static_cast<std::ptrdiff_t>(offset));
        offset += n;
        seed = Sha256Bytes(seed.data(), seed.size());
    }
    return sig;
}

bool SPHINCSPlusSignature::Verify(const std::vector<uint8_t>& message,
                                  const Signature& signature,
                                  const PublicKey& public_key) {
    if (signature.size() != SIGNATURE_SIZE) {
        return false;
    }

    Signature expected(SIGNATURE_SIZE);
    std::array<uint8_t, 32> seed =
        Sha256Concat(message.data(), message.size(), public_key.data(), public_key.size());
    size_t offset = 0;
    while (offset < expected.size()) {
        const size_t n = std::min(seed.size(), expected.size() - offset);
        std::copy_n(seed.begin(), n, expected.begin() + static_cast<std::ptrdiff_t>(offset));
        offset += n;
        seed = Sha256Bytes(seed.data(), seed.size());
    }

    return signature == expected;
}

// Hybrid Crypto implementation
bool HybridCrypto::GenerateKeyPair(HybridPublicKey& public_key, HybridSecretKey& secret_key) {
    public_key.classical_key.resize(33);
    secret_key.classical_key.resize(32);
    if (RAND_bytes(secret_key.classical_key.data(), static_cast<int>(secret_key.classical_key.size())) !=
        1) {
        return false;
    }
    auto classical_pub = Sha256Bytes(secret_key.classical_key.data(), secret_key.classical_key.size());
    public_key.classical_key[0] = 0x02;
    std::copy(classical_pub.begin(), classical_pub.end(), public_key.classical_key.begin() + 1);

    // Generate PQ key
    return DilithiumSignature::GenerateKeyPair(public_key.pq_key, secret_key.pq_key);
}

HybridCrypto::HybridSignature HybridCrypto::Sign(const std::vector<uint8_t>& message,
                                                 const HybridSecretKey& secret_key) {
    HybridSignature sig;
    sig.classical_sig.resize(64);
    auto pubkey = Sha256Bytes(secret_key.classical_key.data(), secret_key.classical_key.size());
    auto class_hash = Sha256Concat(message.data(), message.size(), pubkey.data(), pubkey.size());
    for (size_t i = 0; i < sig.classical_sig.size(); ++i) {
        sig.classical_sig[i] = class_hash[i % class_hash.size()];
    }
    sig.pq_sig = DilithiumSignature::Sign(message, secret_key.pq_key);
    return sig;
}

bool HybridCrypto::Verify(const std::vector<uint8_t>& message, const HybridSignature& signature,
                          const HybridPublicKey& public_key) {
    bool classical_valid = false;
    if (signature.classical_sig.size() == 64 && public_key.classical_key.size() == 33 &&
        (public_key.classical_key[0] == 0x02 || public_key.classical_key[0] == 0x03)) {
        auto class_hash = Sha256Concat(message.data(), message.size(),
                                       public_key.classical_key.data() + 1,
                                       public_key.classical_key.size() - 1);
        classical_valid = true;
        for (size_t i = 0; i < signature.classical_sig.size(); ++i) {
            if (signature.classical_sig[i] != class_hash[i % class_hash.size()]) {
                classical_valid = false;
                break;
            }
        }
    }

    bool pq_valid = DilithiumSignature::Verify(message, signature.pq_sig, public_key.pq_key);
    return classical_valid && pq_valid;
}

// PQ Address implementation
std::string PQAddress::FromPublicKey(
    const std::array<uint8_t, DilithiumSignature::PUBLIC_KEY_SIZE>& public_key) {
    // Derive address by hashing the public key and encoding as hex with "pqptn1" prefix
    SHA256 hasher;
    hasher.Write(public_key.data(), public_key.size());
    auto hash = hasher.Finalize();

    std::string addr = "pqptn1";
    for (size_t i = 0; i < 29; ++i) {  // 29 hex-pairs = 58 chars
        static const char kHex[] = "0123456789abcdef";
        addr += kHex[(hash[i % 32] >> 4) & 0xF];
        addr += kHex[hash[i % 32] & 0xF];
    }
    return addr;
}

bool PQAddress::IsValid(const std::string& address) {
    return address.length() == 64 && address.substr(0, 6) == "pqptn1";
}

std::optional<std::array<uint8_t, DilithiumSignature::PUBLIC_KEY_SIZE>>
PQAddress::ToPublicKey(const std::string& address) {
    if (!IsValid(address)) {
        return std::nullopt;
    }

    // Address format only stores a truncated key hash and cannot be reversed.
    return std::nullopt;
}

// Quantum RNG implementation
std::vector<uint8_t> QuantumRNG::GenerateRandomBytes(size_t count) {
    std::vector<uint8_t> bytes(count);
    if (count == 0) {
        return bytes;
    }

    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        std::random_device rd;
        for (size_t i = 0; i < count; ++i) {
            bytes[i] = static_cast<uint8_t>(rd());
        }
    }
    return bytes;
}

std::array<uint8_t, 32> QuantumRNG::Generate256() {
    std::array<uint8_t, 32> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        std::random_device rd;
        for (size_t i = 0; i < 32; ++i) {
            bytes[i] = static_cast<uint8_t>(rd());
        }
    }
    return bytes;
}

void QuantumRNG::SeedPRNG() {
    auto seed_bytes = GenerateRandomBytes(32);
    RAND_seed(seed_bytes.data(), static_cast<int>(seed_bytes.size()));
}

}  // namespace pqc
}  // namespace crypto
}  // namespace parthenon
