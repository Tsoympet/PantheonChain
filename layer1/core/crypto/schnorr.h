// ParthenonChain - Schnorr Signatures (BIP-340)
// Consensus-critical implementation
// Uses secp256k1 curve with BIP-340 specification

#ifndef PARTHENON_CRYPTO_SCHNORR_H
#define PARTHENON_CRYPTO_SCHNORR_H

#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace crypto {

/**
 * Schnorr signature implementation following BIP-340
 * Uses secp256k1 curve
 * All operations are deterministic and consensus-critical
 */
class Schnorr {
  public:
    static constexpr size_t PRIVATE_KEY_SIZE = 32;
    static constexpr size_t PUBLIC_KEY_SIZE = 32;       // x-only public key
    static constexpr size_t SIGNATURE_SIZE = 64;        // (r, s) where r and s are 32 bytes each
    static constexpr size_t FULL_PUBLIC_KEY_SIZE = 33;  // Compressed public key (with prefix)

    using PrivateKey = std::array<uint8_t, PRIVATE_KEY_SIZE>;
    using PublicKey = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using Signature = std::array<uint8_t, SIGNATURE_SIZE>;

    /**
     * Generate a public key from a private key
     * Returns x-only public key (32 bytes) as per BIP-340
     */
    static std::optional<PublicKey> GetPublicKey(const PrivateKey& privkey);

    /**
     * Sign a message hash using Schnorr signature (BIP-340)
     * msg_hash must be exactly 32 bytes
     * Returns signature (r || s) where r and s are 32 bytes each
     */
    static std::optional<Signature>
    Sign(const PrivateKey& privkey, const uint8_t* msg_hash,
         const uint8_t* aux_rand = nullptr  // Optional 32-byte auxiliary randomness
    );

    /**
     * Verify a Schnorr signature
     * msg_hash must be exactly 32 bytes
     */
    static bool Verify(const PublicKey& pubkey, const uint8_t* msg_hash,
                       const Signature& signature);

    /**
     * Validate a private key
     * Must be in range [1, n-1] where n is the curve order
     */
    static bool ValidatePrivateKey(const PrivateKey& privkey);

    /**
     * Validate a public key
     * Must be a valid x-only public key on the curve
     */
    static bool ValidatePublicKey(const PublicKey& pubkey);

  private:
    // Internal secp256k1 context management
    static void* GetContext();
    static void CleanupContext();
};

}  // namespace crypto
}  // namespace parthenon

#endif  // PARTHENON_CRYPTO_SCHNORR_H
