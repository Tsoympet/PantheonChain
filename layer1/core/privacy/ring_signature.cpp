#include "ring_signature.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace privacy {

// RingSigner Implementation
RingSignature RingSigner::Sign(const std::vector<uint8_t>& message,
                               const std::vector<std::array<uint8_t, 33>>& ring_keys,
                               const std::array<uint8_t, 32>& secret_key, size_t secret_index) {
    RingSignature sig;

    if (ring_keys.empty() || secret_index >= ring_keys.size()) {
        return sig;  // Invalid
    }

    sig.ring = ring_keys;
    sig.key_image = GenerateKeyImage(secret_key, ring_keys[secret_index]);

    // Generate random values for ring
    sig.signatures.resize(ring_keys.size());

    // Simplified ring signature generation
    // In production, would use proper ring signature algorithm (e.g., CLSAG)
    for (size_t i = 0; i < ring_keys.size(); ++i) {
        crypto::SHA256 hasher;
        hasher.Write(message.data(), message.size());
        hasher.Write(ring_keys[i].data(), ring_keys[i].size());
        hasher.Write(secret_key.data(), secret_key.size());

        std::array<uint8_t, 32> hash;
        hash = hasher.Finalize();

        // Store in signature (simplified)
        std::memcpy(sig.signatures[i].data(), hash.data(), 32);
        std::memcpy(sig.signatures[i].data() + 32, hash.data(), 32);
    }

    return sig;
}

std::array<uint8_t, 32> RingSigner::GenerateKeyImage(const std::array<uint8_t, 32>& secret_key,
                                                     const std::array<uint8_t, 33>& public_key) {
    // Key Image = secret_key * HashToPoint(public_key)
    // Simplified implementation
    std::array<uint8_t, 32> key_image;

    auto hash_point = HashToPoint(public_key);

    // XOR secret key with hash point (simplified)
    for (size_t i = 0; i < 32; ++i) {
        key_image[i] = secret_key[i] ^ hash_point[i];
    }

    return key_image;
}

std::array<uint8_t, 32> RingSigner::HashToPoint(const std::array<uint8_t, 33>& public_key) {
    std::array<uint8_t, 32> point;

    crypto::SHA256 hasher;
    hasher.Write(public_key.data(), public_key.size());
    point = hasher.Finalize();

    return point;
}

// RingVerifier Implementation
bool RingVerifier::Verify(const RingSignature& signature, const std::vector<uint8_t>& message) {
    if (!signature.IsValid()) {
        return false;
    }

    if (signature.signatures.size() != signature.ring.size()) {
        return false;
    }

    // Verify each signature component
    for (size_t i = 0; i < signature.ring.size(); ++i) {
        // Simplified verification
        // In production, would verify ring signature equation
        bool valid = true;
        for (uint8_t byte : signature.signatures[i]) {
            if (byte == 0) {
                valid = false;
                break;
            }
        }

        if (!valid) {
            return false;
        }
    }

    return true;
}

bool RingVerifier::CheckKeyImageUniqueness(
    const std::array<uint8_t, 32>& key_image,
    const std::vector<std::array<uint8_t, 32>>& used_key_images) {
    // Check if key image already exists
    for (const auto& used : used_key_images) {
        if (key_image == used) {
            return false;  // Already used (double spend attempt)
        }
    }

    return true;  // Unique
}

// LinkableRingSignature Implementation
RingSignature LinkableRingSignature::SignLinkable(
    const std::vector<uint8_t>& message, const std::vector<std::array<uint8_t, 33>>& ring_keys,
    const std::array<uint8_t, 32>& secret_key, size_t secret_index) {
    // LSAG signature includes key image for linkability
    return RingSigner::Sign(message, ring_keys, secret_key, secret_index);
}

bool LinkableRingSignature::VerifyLinkable(const RingSignature& signature,
                                           const std::vector<uint8_t>& message) {
    // Verify base ring signature
    if (!RingVerifier::Verify(signature, message)) {
        return false;
    }

    // Verify key image is valid
    bool key_image_valid = false;
    for (uint8_t byte : signature.key_image) {
        if (byte != 0) {
            key_image_valid = true;
            break;
        }
    }

    return key_image_valid;
}

bool LinkableRingSignature::AreLinked(const RingSignature& sig1, const RingSignature& sig2) {
    // Two signatures are linked if they have the same key image
    return sig1.key_image == sig2.key_image;
}

// StealthAddress Implementation
std::array<uint8_t, 33> StealthAddress::Generate(const std::array<uint8_t, 33>& view_key,
                                                 const std::array<uint8_t, 33>& spend_key,
                                                 const std::array<uint8_t, 32>& random) {
    std::array<uint8_t, 33> stealth_addr;

    // Stealth address = Hash(random * view_key) * G + spend_key
    // Simplified implementation

    crypto::SHA256 hasher;
    hasher.Write(random.data(), random.size());
    hasher.Write(view_key.data(), view_key.size());

    std::array<uint8_t, 32> hash;
    hash = hasher.Finalize();

    // Combine with spend key
    stealth_addr[0] = 0x02;  // Compressed public key prefix
    for (size_t i = 0; i < 32; ++i) {
        stealth_addr[i + 1] = hash[i] ^ spend_key[i + 1];
    }

    return stealth_addr;
}

bool StealthAddress::BelongsTo(const std::array<uint8_t, 33>& stealth_addr,
                               const std::array<uint8_t, 32>& view_secret,
                               const std::array<uint8_t, 33>& spend_public,
                               const std::array<uint8_t, 33>& tx_public_key) {
    // Compute what the stealth address should be
    crypto::SHA256 hasher;
    hasher.Write(view_secret.data(), view_secret.size());
    hasher.Write(tx_public_key.data(), tx_public_key.size());

    std::array<uint8_t, 32> hash;
    hash = hasher.Finalize();

    // Check if it matches
    std::array<uint8_t, 33> computed;
    computed[0] = 0x02;
    for (size_t i = 0; i < 32; ++i) {
        computed[i + 1] = hash[i] ^ spend_public[i + 1];
    }

    return stealth_addr == computed;
}

std::array<uint8_t, 32>
StealthAddress::RecoverSecretKey(const std::array<uint8_t, 32>& view_secret,
                                 const std::array<uint8_t, 32>& spend_secret,
                                 const std::array<uint8_t, 33>& tx_public_key) {
    std::array<uint8_t, 32> secret_key;

    // Compute shared secret
    crypto::SHA256 hasher;
    hasher.Write(view_secret.data(), view_secret.size());
    hasher.Write(tx_public_key.data(), tx_public_key.size());

    std::array<uint8_t, 32> shared_secret;
    shared_secret = hasher.Finalize();

    // Derive stealth secret key = spend_secret + shared_secret
    for (size_t i = 0; i < 32; ++i) {
        secret_key[i] = spend_secret[i] ^ shared_secret[i];
    }

    return secret_key;
}

}  // namespace privacy
}  // namespace parthenon
