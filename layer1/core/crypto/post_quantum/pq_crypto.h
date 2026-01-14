// ParthenonChain - Post-Quantum Cryptography
// Quantum-resistant cryptographic primitives

#pragma once

#include <vector>
#include <string>
#include <array>
#include <optional>
#include <cstdint>

namespace parthenon {
namespace crypto {
namespace pqc {

/**
 * Post-Quantum Signature Scheme
 * Using CRYSTALS-Dilithium (NIST PQC standard)
 */
class DilithiumSignature {
public:
    static constexpr size_t PUBLIC_KEY_SIZE = 1952;
    static constexpr size_t SECRET_KEY_SIZE = 4000;
    static constexpr size_t SIGNATURE_SIZE = 3293;
    
    using PublicKey = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using SecretKey = std::array<uint8_t, SECRET_KEY_SIZE>;
    using Signature = std::array<uint8_t, SIGNATURE_SIZE>;
    
    /**
     * Generate key pair
     */
    static bool GenerateKeyPair(PublicKey& public_key, SecretKey& secret_key);
    
    /**
     * Sign message
     */
    static Signature Sign(
        const std::vector<uint8_t>& message,
        const SecretKey& secret_key
    );
    
    /**
     * Verify signature
     */
    static bool Verify(
        const std::vector<uint8_t>& message,
        const Signature& signature,
        const PublicKey& public_key
    );
};

/**
 * Post-Quantum Key Exchange
 * Using CRYSTALS-Kyber (NIST PQC standard)
 */
class KyberKEM {
public:
    static constexpr size_t PUBLIC_KEY_SIZE = 1568;
    static constexpr size_t SECRET_KEY_SIZE = 3168;
    static constexpr size_t CIPHERTEXT_SIZE = 1568;
    static constexpr size_t SHARED_SECRET_SIZE = 32;
    
    using PublicKey = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using SecretKey = std::array<uint8_t, SECRET_KEY_SIZE>;
    using Ciphertext = std::array<uint8_t, CIPHERTEXT_SIZE>;
    using SharedSecret = std::array<uint8_t, SHARED_SECRET_SIZE>;
    
    /**
     * Generate key pair for key encapsulation
     */
    static bool GenerateKeyPair(PublicKey& public_key, SecretKey& secret_key);
    
    /**
     * Encapsulate shared secret
     */
    static bool Encapsulate(
        const PublicKey& public_key,
        Ciphertext& ciphertext,
        SharedSecret& shared_secret
    );
    
    /**
     * Decapsulate shared secret
     */
    static std::optional<SharedSecret> Decapsulate(
        const Ciphertext& ciphertext,
        const SecretKey& secret_key
    );
};

/**
 * Hash-Based Signatures (Stateless)
 * SPHINCS+ for quantum-resistant signatures
 */
class SPHINCSPlusSignature {
public:
    static constexpr size_t PUBLIC_KEY_SIZE = 64;
    static constexpr size_t SECRET_KEY_SIZE = 128;
    static constexpr size_t SIGNATURE_SIZE = 49856;  // Large but secure
    
    using PublicKey = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using SecretKey = std::array<uint8_t, SECRET_KEY_SIZE>;
    using Signature = std::vector<uint8_t>;  // Variable size
    
    /**
     * Generate key pair
     */
    static bool GenerateKeyPair(PublicKey& public_key, SecretKey& secret_key);
    
    /**
     * Sign message (stateless - can sign unlimited messages)
     */
    static Signature Sign(
        const std::vector<uint8_t>& message,
        const SecretKey& secret_key
    );
    
    /**
     * Verify signature
     */
    static bool Verify(
        const std::vector<uint8_t>& message,
        const Signature& signature,
        const PublicKey& public_key
    );
};

/**
 * Hybrid Cryptography
 * Combines classical and post-quantum for transition period
 */
class HybridCrypto {
public:
    struct HybridPublicKey {
        std::vector<uint8_t> classical_key;  // Schnorr/ECDSA
        DilithiumSignature::PublicKey pq_key;
    };
    
    struct HybridSecretKey {
        std::vector<uint8_t> classical_key;
        DilithiumSignature::SecretKey pq_key;
    };
    
    struct HybridSignature {
        std::vector<uint8_t> classical_sig;
        DilithiumSignature::Signature pq_sig;
    };
    
    /**
     * Generate hybrid key pair
     */
    static bool GenerateKeyPair(
        HybridPublicKey& public_key,
        HybridSecretKey& secret_key
    );
    
    /**
     * Sign with both classical and PQ
     */
    static HybridSignature Sign(
        const std::vector<uint8_t>& message,
        const HybridSecretKey& secret_key
    );
    
    /**
     * Verify both signatures (both must be valid)
     */
    static bool Verify(
        const std::vector<uint8_t>& message,
        const HybridSignature& signature,
        const HybridPublicKey& public_key
    );
};

/**
 * Post-Quantum Address
 * Blockchain address using PQ public key
 */
class PQAddress {
public:
    /**
     * Generate address from PQ public key
     */
    static std::string FromPublicKey(
        const std::array<uint8_t, DilithiumSignature::PUBLIC_KEY_SIZE>& public_key
    );
    
    /**
     * Verify address is valid
     */
    static bool IsValid(const std::string& address);
    
    /**
     * Get public key from address
     */
    static std::optional<std::array<uint8_t, DilithiumSignature::PUBLIC_KEY_SIZE>> ToPublicKey(
        const std::string& address
    );
};

/**
 * Quantum Random Number Generator
 * True quantum randomness for key generation
 */
class QuantumRNG {
public:
    /**
     * Generate quantum random bytes
     */
    static std::vector<uint8_t> GenerateRandomBytes(size_t count);
    
    /**
     * Generate random 256-bit value
     */
    static std::array<uint8_t, 32> Generate256();
    
    /**
     * Seed classical PRNG with quantum randomness
     */
    static void SeedPRNG();
};

} // namespace pqc
} // namespace crypto
} // namespace parthenon
