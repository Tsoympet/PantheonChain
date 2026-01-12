#ifndef PARTHENON_SETTLEMENT_MULTISIG_H
#define PARTHENON_SETTLEMENT_MULTISIG_H

#include <cstdint>
#include <vector>
#include <array>

namespace parthenon {
namespace settlement {

// Public key for multisig (33-byte compressed secp256k1 key)
using PubKey = std::array<uint8_t, 33>;

// Schnorr signature (64 bytes)
using Signature = std::array<uint8_t, 64>;

// Multisig policy: M-of-N signatures required
class MultisigPolicy {
public:
    static constexpr size_t MAX_KEYS = 15;
    
    MultisigPolicy();
    MultisigPolicy(uint8_t m, const std::vector<PubKey>& pubkeys);
    
    uint8_t GetM() const { return m_; }
    uint8_t GetN() const { return static_cast<uint8_t>(pubkeys_.size()); }
    const std::vector<PubKey>& GetPubKeys() const { return pubkeys_; }
    
    bool IsValid() const;
    bool AddPubKey(const PubKey& pubkey);
    
    std::vector<uint8_t> Serialize() const;
    static MultisigPolicy Deserialize(const std::vector<uint8_t>& data, size_t& pos);

private:
    uint8_t m_;  // Required signatures
    std::vector<PubKey> pubkeys_;  // Public keys (N keys)
};

// Aggregated signature for multisig
class AggregatedSignature {
public:
    AggregatedSignature();
    
    void AddSignature(uint8_t key_index, const Signature& sig);
    const std::vector<std::pair<uint8_t, Signature>>& GetSignatures() const { return signatures_; }
    
    size_t GetSignatureCount() const { return signatures_.size(); }
    bool HasSignature(uint8_t key_index) const;
    
    std::vector<uint8_t> Serialize() const;
    static AggregatedSignature Deserialize(const std::vector<uint8_t>& data, size_t& pos);

private:
    std::vector<std::pair<uint8_t, Signature>> signatures_;  // (key_index, signature) pairs
};

// Multisig validator
class MultisigValidator {
public:
    // Verify that aggregated signature satisfies multisig policy
    static bool VerifySignatures(
        const MultisigPolicy& policy,
        const AggregatedSignature& agg_sig,
        const std::vector<uint8_t>& message);
    
    // Verify a single Schnorr signature
    static bool VerifySchnorrSignature(
        const PubKey& pubkey,
        const Signature& sig,
        const std::vector<uint8_t>& message);

private:
    static bool ValidateKeyIndex(uint8_t key_index, uint8_t n);
};

} // namespace settlement
} // namespace parthenon

#endif // PARTHENON_SETTLEMENT_MULTISIG_H
