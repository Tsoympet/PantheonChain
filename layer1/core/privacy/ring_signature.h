#ifndef PARTHENON_CORE_PRIVACY_RING_SIGNATURE_H
#define PARTHENON_CORE_PRIVACY_RING_SIGNATURE_H

#include <cstdint>
#include <vector>
#include <array>

namespace parthenon {
namespace privacy {

/**
 * Ring Signature
 * Signature that proves signer is one of a group without revealing which one
 */
struct RingSignature {
    std::vector<std::array<uint8_t, 64>> signatures;  // c values and s values
    std::array<uint8_t, 32> key_image;                // Prevents double-signing
    std::vector<std::array<uint8_t, 33>> ring;        // Public keys in the ring
    
    bool IsValid() const { 
        return !signatures.empty() && !ring.empty(); 
    }
};

/**
 * Ring Signature Signer
 * Creates ring signatures for anonymous signing
 */
class RingSigner {
public:
    /**
     * Sign message with ring signature
     * 
     * @param message Message to sign
     * @param ring_keys Public keys in the anonymity set
     * @param secret_key Actual signer's secret key
     * @param secret_index Index of signer's public key in ring
     */
    static RingSignature Sign(
        const std::vector<uint8_t>& message,
        const std::vector<std::array<uint8_t, 33>>& ring_keys,
        const std::array<uint8_t, 32>& secret_key,
        size_t secret_index
    );
    
    /**
     * Generate key image
     * Key image uniquely identifies the signer without revealing identity
     */
    static std::array<uint8_t, 32> GenerateKeyImage(
        const std::array<uint8_t, 32>& secret_key,
        const std::array<uint8_t, 33>& public_key
    );
    
private:
    static std::array<uint8_t, 32> HashToPoint(
        const std::array<uint8_t, 33>& public_key
    );
};

/**
 * Ring Signature Verifier
 * Verifies ring signatures
 */
class RingVerifier {
public:
    /**
     * Verify ring signature
     * 
     * @param signature Ring signature to verify
     * @param message Original message
     * @return true if signature is valid
     */
    static bool Verify(
        const RingSignature& signature,
        const std::vector<uint8_t>& message
    );
    
    /**
     * Check if key image has been used before
     * Used to prevent double-spending in private transactions
     */
    static bool CheckKeyImageUniqueness(
        const std::array<uint8_t, 32>& key_image,
        const std::vector<std::array<uint8_t, 32>>& used_key_images
    );
};

/**
 * Linkable Ring Signature (LSAG)
 * Ring signature that can detect if same key signs twice
 */
class LinkableRingSignature {
public:
    /**
     * Sign with linkable ring signature
     */
    static RingSignature SignLinkable(
        const std::vector<uint8_t>& message,
        const std::vector<std::array<uint8_t, 33>>& ring_keys,
        const std::array<uint8_t, 32>& secret_key,
        size_t secret_index
    );
    
    /**
     * Verify linkable ring signature
     */
    static bool VerifyLinkable(
        const RingSignature& signature,
        const std::vector<uint8_t>& message
    );
    
    /**
     * Check if two signatures are from same signer
     */
    static bool AreLinked(
        const RingSignature& sig1,
        const RingSignature& sig2
    );
};

/**
 * Stealth Address
 * One-time address for receiving payments anonymously
 */
class StealthAddress {
public:
    /**
     * Generate stealth address from recipient's public keys
     * 
     * @param view_key Recipient's public view key
     * @param spend_key Recipient's public spend key
     * @param random Random value chosen by sender
     * @return One-time stealth address
     */
    static std::array<uint8_t, 33> Generate(
        const std::array<uint8_t, 33>& view_key,
        const std::array<uint8_t, 33>& spend_key,
        const std::array<uint8_t, 32>& random
    );
    
    /**
     * Check if stealth address belongs to recipient
     * 
     * @param stealth_addr Stealth address to check
     * @param view_secret Recipient's secret view key
     * @param spend_public Recipient's public spend key
     * @param tx_public_key Public key from transaction
     * @return true if address belongs to recipient
     */
    static bool BelongsTo(
        const std::array<uint8_t, 33>& stealth_addr,
        const std::array<uint8_t, 32>& view_secret,
        const std::array<uint8_t, 33>& spend_public,
        const std::array<uint8_t, 33>& tx_public_key
    );
    
    /**
     * Recover secret key for stealth address
     * 
     * @param view_secret Recipient's secret view key
     * @param spend_secret Recipient's secret spend key
     * @param tx_public_key Public key from transaction
     * @return Secret key for the stealth address
     */
    static std::array<uint8_t, 32> RecoverSecretKey(
        const std::array<uint8_t, 32>& view_secret,
        const std::array<uint8_t, 32>& spend_secret,
        const std::array<uint8_t, 33>& tx_public_key
    );
};

} // namespace privacy
} // namespace parthenon

#endif // PARTHENON_CORE_PRIVACY_RING_SIGNATURE_H
