#include "multisig.h"

#include "../core/crypto/schnorr.h"
#include "../core/crypto/sha256.h"

#include <algorithm>

namespace parthenon {
namespace settlement {

// MultisigPolicy implementation
MultisigPolicy::MultisigPolicy() : m_(0), pubkeys_() {}

MultisigPolicy::MultisigPolicy(uint8_t m, const std::vector<PubKey>& pubkeys)
    : m_(m), pubkeys_(pubkeys) {}

bool MultisigPolicy::IsValid() const {
    // Check M and N constraints
    if (m_ == 0 || m_ > pubkeys_.size()) {
        return false;
    }

    // Check max keys limit
    if (pubkeys_.size() > MAX_KEYS) {
        return false;
    }

    // Check for duplicate keys
    std::vector<PubKey> sorted_keys = pubkeys_;
    std::sort(sorted_keys.begin(), sorted_keys.end());
    for (size_t i = 1; i < sorted_keys.size(); ++i) {
        if (sorted_keys[i] == sorted_keys[i - 1]) {
            return false;
        }
    }

    return true;
}

bool MultisigPolicy::AddPubKey(const PubKey& pubkey) {
    if (pubkeys_.size() >= MAX_KEYS) {
        return false;
    }

    // Check for duplicates
    for (const auto& key : pubkeys_) {
        if (key == pubkey) {
            return false;
        }
    }

    pubkeys_.push_back(pubkey);
    return true;
}

std::vector<uint8_t> MultisigPolicy::Serialize() const {
    std::vector<uint8_t> result;

    // Serialize M
    result.push_back(m_);

    // Serialize N
    result.push_back(static_cast<uint8_t>(pubkeys_.size()));

    // Serialize public keys
    for (const auto& pubkey : pubkeys_) {
        result.insert(result.end(), pubkey.begin(), pubkey.end());
    }

    return result;
}

MultisigPolicy MultisigPolicy::Deserialize(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 2 > data.size()) {
        return MultisigPolicy();
    }

    // Deserialize M
    uint8_t m = data[pos++];

    // Deserialize N
    uint8_t n = data[pos++];

    // Check bounds
    if (n > MAX_KEYS || pos + (n * 33) > data.size()) {
        return MultisigPolicy();
    }

    // Deserialize public keys
    std::vector<PubKey> pubkeys;
    for (uint8_t i = 0; i < n; ++i) {
        PubKey pubkey;
        std::copy(data.begin() + pos, data.begin() + pos + 33, pubkey.begin());
        pubkeys.push_back(pubkey);
        pos += 33;
    }

    return MultisigPolicy(m, pubkeys);
}

// AggregatedSignature implementation
AggregatedSignature::AggregatedSignature() : signatures_() {}

void AggregatedSignature::AddSignature(uint8_t key_index, const Signature& sig) {
    // Check for duplicate index
    for (const auto& pair : signatures_) {
        if (pair.first == key_index) {
            return;  // Already have signature for this key
        }
    }

    signatures_.emplace_back(key_index, sig);
}

bool AggregatedSignature::HasSignature(uint8_t key_index) const {
    for (const auto& pair : signatures_) {
        if (pair.first == key_index) {
            return true;
        }
    }
    return false;
}

std::vector<uint8_t> AggregatedSignature::Serialize() const {
    std::vector<uint8_t> result;

    // Serialize signature count
    result.push_back(static_cast<uint8_t>(signatures_.size()));

    // Serialize each signature with its index
    for (const auto& pair : signatures_) {
        result.push_back(pair.first);                                         // Key index
        result.insert(result.end(), pair.second.begin(), pair.second.end());  // Signature
    }

    return result;
}

AggregatedSignature AggregatedSignature::Deserialize(const std::vector<uint8_t>& data,
                                                     size_t& pos) {
    if (pos + 1 > data.size()) {
        return AggregatedSignature();
    }

    // Deserialize signature count
    uint8_t count = data[pos++];

    // Check bounds
    if (count > MultisigPolicy::MAX_KEYS || pos + (count * 65) > data.size()) {
        return AggregatedSignature();
    }

    AggregatedSignature agg_sig;

    // Deserialize each signature
    for (uint8_t i = 0; i < count; ++i) {
        uint8_t key_index = data[pos++];

        Signature sig;
        std::copy(data.begin() + pos, data.begin() + pos + 64, sig.begin());
        pos += 64;

        agg_sig.AddSignature(key_index, sig);
    }

    return agg_sig;
}

// MultisigValidator implementation
bool MultisigValidator::ValidateKeyIndex(uint8_t key_index, uint8_t n) {
    return key_index < n;
}

bool MultisigValidator::VerifySchnorrSignature(const PubKey& pubkey, const Signature& sig,
                                               const std::vector<uint8_t>& message) {
    // Hash the message first
    auto msg_hash = crypto::SHA256::Hash256(message.data(), message.size());

    // Convert 33-byte compressed pubkey to 32-byte x-only pubkey.
    // A compressed secp256k1 public key is: [0x02|0x03] || X (33 bytes).
    // BIP340 (Schnorr) uses only the X coordinate (32 bytes), so we skip the
    // first prefix byte.  The parity (even/odd Y) is implicitly handled by the
    // Schnorr signing convention; the verifier assumes the even-Y point when the
    // prefix is 0x02, which is the canonical form used throughout this codebase.
    crypto::Schnorr::PublicKey xonly_pubkey;
    std::copy(pubkey.begin() + 1, pubkey.end(), xonly_pubkey.begin());

    crypto::Schnorr::Signature schnorr_sig;
    std::copy(sig.begin(), sig.end(), schnorr_sig.begin());

    // Verify using Schnorr::Verify
    return crypto::Schnorr::Verify(xonly_pubkey, msg_hash.data(), schnorr_sig);
}

bool MultisigValidator::VerifySignatures(const MultisigPolicy& policy,
                                         const AggregatedSignature& agg_sig,
                                         const std::vector<uint8_t>& message) {
    // Check if policy is valid
    if (!policy.IsValid()) {
        return false;
    }

    // Check if we have enough signatures
    if (agg_sig.GetSignatureCount() < policy.GetM()) {
        return false;
    }

    const auto& pubkeys = policy.GetPubKeys();
    const auto& signatures = agg_sig.GetSignatures();

    uint8_t valid_signatures = 0;

    // Verify each signature
    for (const auto& pair : signatures) {
        uint8_t key_index = pair.first;
        const Signature& sig = pair.second;

        // Validate key index
        if (!ValidateKeyIndex(key_index, policy.GetN())) {
            return false;
        }

        // Get corresponding public key
        const PubKey& pubkey = pubkeys[key_index];

        // Verify signature
        if (VerifySchnorrSignature(pubkey, sig, message)) {
            valid_signatures++;
        }
    }

    // Check if we have enough valid signatures
    return valid_signatures >= policy.GetM();
}

}  // namespace settlement
}  // namespace parthenon
