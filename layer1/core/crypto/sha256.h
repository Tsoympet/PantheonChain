// ParthenonChain - Consensus-Critical Cryptographic Primitives
// SHA-256 Implementation
// MUST be deterministic and consensus-safe

#ifndef PARTHENON_CRYPTO_SHA256_H
#define PARTHENON_CRYPTO_SHA256_H

#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <string>

namespace parthenon {
namespace crypto {

/**
 * SHA-256 hasher
 * Implements the SHA-256 cryptographic hash function as specified in FIPS 180-4
 * This is a consensus-critical component - any changes must maintain compatibility
 */
class SHA256 {
public:
    static constexpr size_t OUTPUT_SIZE = 32;
    static constexpr size_t BLOCK_SIZE = 64;

    using Hash = std::array<uint8_t, OUTPUT_SIZE>;

    SHA256();
    ~SHA256();

    // Reset the hasher to initial state
    void Reset();

    // Update the hash with new data
    void Write(const uint8_t* data, size_t len);
    void Write(const std::vector<uint8_t>& data);

    // Finalize and return the hash
    Hash Finalize();

    // Convenience function: hash data in one call
    static Hash Hash256(const uint8_t* data, size_t len);
    static Hash Hash256(const std::vector<uint8_t>& data);

private:
    void Transform(const uint8_t* chunk);
    
    uint32_t state_[8];
    uint8_t buffer_[BLOCK_SIZE];
    uint64_t byte_count_;
    size_t buffer_size_;
};

/**
 * SHA-256d (Double SHA-256)
 * Used for block hashing and proof-of-work
 * SHA256d(x) = SHA256(SHA256(x))
 */
class SHA256d {
public:
    using Hash = SHA256::Hash;

    // Compute double SHA-256
    static Hash Hash256d(const uint8_t* data, size_t len);
    static Hash Hash256d(const std::vector<uint8_t>& data);
};

/**
 * Tagged SHA-256
 * Implements BIP-340 style tagged hashing
 * TaggedHash(tag, msg) = SHA256(SHA256(tag) || SHA256(tag) || msg)
 */
class TaggedSHA256 {
public:
    using Hash = SHA256::Hash;

    TaggedSHA256(const std::string& tag);
    
    void Write(const uint8_t* data, size_t len);
    void Write(const std::vector<uint8_t>& data);
    
    Hash Finalize();

    // Convenience function
    static Hash HashTagged(const std::string& tag, const uint8_t* data, size_t len);
    static Hash HashTagged(const std::string& tag, const std::vector<uint8_t>& data);

private:
    SHA256 hasher_;
};

} // namespace crypto
} // namespace parthenon

#endif // PARTHENON_CRYPTO_SHA256_H
