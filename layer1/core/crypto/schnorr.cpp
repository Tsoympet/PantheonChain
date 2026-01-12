// ParthenonChain - Schnorr Signature Implementation
// BIP-340 compliant using secp256k1
// Consensus-critical: DO NOT MODIFY

#include "schnorr.h"
#include "sha256.h"
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <cstring>
#include <mutex>

namespace parthenon {
namespace crypto {

// Global context for secp256k1 operations
static std::mutex context_mutex;
static secp256k1_context* global_context = nullptr;
static int context_ref_count = 0;

void* Schnorr::GetContext() {
    std::lock_guard<std::mutex> lock(context_mutex);
    if (global_context == nullptr) {
        global_context = secp256k1_context_create(
            SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY
        );
    }
    context_ref_count++;
    return global_context;
}

void Schnorr::CleanupContext() {
    std::lock_guard<std::mutex> lock(context_mutex);
    if (context_ref_count > 0) {
        context_ref_count--;
    }
    if (context_ref_count == 0 && global_context != nullptr) {
        secp256k1_context_destroy(global_context);
        global_context = nullptr;
    }
}

std::optional<Schnorr::PublicKey> Schnorr::GetPublicKey(const PrivateKey& privkey) {
    if (!ValidatePrivateKey(privkey)) {
        return std::nullopt;
    }
    
    auto ctx = static_cast<secp256k1_context*>(GetContext());
    
    secp256k1_keypair keypair;
    if (!secp256k1_keypair_create(ctx, &keypair, privkey.data())) {
        return std::nullopt;
    }
    
    secp256k1_xonly_pubkey xonly_pubkey;
    if (!secp256k1_keypair_xonly_pub(ctx, &xonly_pubkey, nullptr, &keypair)) {
        return std::nullopt;
    }
    
    PublicKey pubkey;
    if (!secp256k1_xonly_pubkey_serialize(ctx, pubkey.data(), &xonly_pubkey)) {
        return std::nullopt;
    }
    
    return pubkey;
}

std::optional<Schnorr::Signature> Schnorr::Sign(
    const PrivateKey& privkey,
    const uint8_t* msg_hash,
    const uint8_t* aux_rand
) {
    if (!ValidatePrivateKey(privkey)) {
        return std::nullopt;
    }
    
    auto ctx = static_cast<secp256k1_context*>(GetContext());
    
    secp256k1_keypair keypair;
    if (!secp256k1_keypair_create(ctx, &keypair, privkey.data())) {
        return std::nullopt;
    }
    
    Signature signature;
    if (!secp256k1_schnorrsig_sign32(ctx, signature.data(), msg_hash, &keypair, aux_rand)) {
        return std::nullopt;
    }
    
    return signature;
}

bool Schnorr::Verify(
    const PublicKey& pubkey,
    const uint8_t* msg_hash,
    const Signature& signature
) {
    if (!ValidatePublicKey(pubkey)) {
        return false;
    }
    
    auto ctx = static_cast<secp256k1_context*>(GetContext());
    
    secp256k1_xonly_pubkey xonly_pubkey;
    if (!secp256k1_xonly_pubkey_parse(ctx, &xonly_pubkey, pubkey.data())) {
        return false;
    }
    
    return secp256k1_schnorrsig_verify(ctx, signature.data(), msg_hash, 32, &xonly_pubkey) == 1;
}

bool Schnorr::ValidatePrivateKey(const PrivateKey& privkey) {
    auto ctx = static_cast<secp256k1_context*>(GetContext());
    return secp256k1_ec_seckey_verify(ctx, privkey.data()) == 1;
}

bool Schnorr::ValidatePublicKey(const PublicKey& pubkey) {
    auto ctx = static_cast<secp256k1_context*>(GetContext());
    
    secp256k1_xonly_pubkey xonly_pubkey;
    return secp256k1_xonly_pubkey_parse(ctx, &xonly_pubkey, pubkey.data()) == 1;
}

} // namespace crypto
} // namespace parthenon
