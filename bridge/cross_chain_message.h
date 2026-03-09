#pragma once
// cross_chain_message.h — Canonical cross-chain message standard for PantheonChain
//
// All cross-chain messages (OBOLOS ↔ DRACHMA ↔ TALANTON) MUST use this format.
// Messages include replay protection via nonce tracking and Merkle proof verification.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace pantheon {
namespace bridge {

// Chain identifiers — must match genesis chain_id values
enum class ChainId : uint32_t {
    TALANTON = 1001,
    DRACHMA  = 1002,
    OBOLOS   = 1003,
};

// Maximum payload size for cross-chain messages (32 KB)
static constexpr size_t kMaxPayloadBytes = 32 * 1024;

// 32-byte hash type
using Hash256 = std::array<uint8_t, 32>;

// Validator signature (64-byte Schnorr signature + 33-byte compressed pubkey)
struct ValidatorSignature {
    std::array<uint8_t, 64> signature;    // Schnorr signature
    std::array<uint8_t, 33> public_key;   // Compressed secp256k1 pubkey
};

// Cross-chain message — canonical format per the PantheonChain protocol spec.
//
// All fields are required. The nonce provides replay protection: each
// (origin_chain_id, destination_chain_id, message_nonce) triple MUST be
// processed exactly once on the destination chain.
struct CrossChainMessage {
    // ── Identity ─────────────────────────────────────────────────────
    ChainId  origin_chain_id;       // Chain that produced this message
    ChainId  destination_chain_id;  // Chain that must process this message
    uint64_t message_nonce;         // Monotonically increasing per sender address
    uint64_t block_height;          // Block height on origin chain
    uint64_t timestamp;             // Unix timestamp of origin block

    // ── Payload ──────────────────────────────────────────────────────
    Hash256              payload_hash;  // SHA256d of the serialised payload
    std::vector<uint8_t> payload;       // Raw payload bytes (≤ kMaxPayloadBytes)

    // ── Proof ────────────────────────────────────────────────────────
    Hash256                         state_root;  // MPT state root at block_height
    std::vector<std::vector<uint8_t>> proof;     // Merkle inclusion proof nodes

    // ── Signatures ───────────────────────────────────────────────────
    // Must contain ≥ ⌈2/3⌉ of the active validator set by stake weight.
    std::vector<ValidatorSignature> validator_signatures;
};

// Checkpoint — periodic state commitment anchored up the security hierarchy.
//
// OBOLOS commits checkpoints → DRACHMA.
// DRACHMA commits checkpoints → TALANTON.
//
// Each checkpoint contains enough information for the parent chain to
// verify the child chain's state without replaying all transactions.
struct Checkpoint {
    ChainId  source_chain_id;   // Chain that produced this checkpoint
    ChainId  target_chain_id;   // Chain that stores this checkpoint
    uint64_t height;            // Block height on source chain
    uint64_t timestamp;         // Unix timestamp of source block
    Hash256  block_hash;        // Hash of the source block header
    Hash256  state_root;        // MPT state root at height
    uint64_t commitment_nonce;  // Monotonic counter for this (source, target) pair

    // Validator signatures over (source_chain_id ‖ height ‖ block_hash ‖ state_root)
    // Must have ≥ ⌈2/3⌉ by stake weight.
    std::vector<ValidatorSignature> validator_signatures;
};

// BridgeTransferIntent — locked asset record for the lock-mint bridge.
//
// Lock-mint flow (canonical):
//   1. Sender locks tokens on the origin chain (creates this record on-chain).
//   2. Relayer observes the lock, generates a CrossChainMessage.
//   3. Destination chain verifies Merkle proof and mints wrapped tokens.
//
// Burn-unlock flow (reverse):
//   1. Sender burns wrapped tokens on the destination chain.
//   2. Relayer observes the burn, generates a CrossChainMessage.
//   3. Origin chain verifies Merkle proof and unlocks native tokens.
struct BridgeTransferIntent {
    ChainId     origin_chain_id;       // Chain where native tokens are locked
    ChainId     destination_chain_id;  // Chain where wrapped tokens are minted
    std::string sender_address;        // Address on origin chain (address_prefix encoded)
    std::string recipient_address;     // Address on destination chain
    std::string token_symbol;          // Native token symbol (TLT / DRC / OBL)
    std::string wrapped_token_symbol;  // Wrapped token symbol (wTLT / wDRC)
    uint64_t    amount_base_units;     // Amount in smallest indivisible units
    uint64_t    nonce;                 // Replay-protection nonce (per sender)
    Hash256     lock_tx_hash;          // Transaction hash of the lock operation
    uint64_t    lock_block_height;     // Block height when tokens were locked
};

// FraudProof — evidence of invalid state transition for dispute resolution.
struct FraudProof {
    ChainId  disputed_chain_id;   // Chain where the invalid transition occurred
    uint64_t disputed_height;     // Block height of the invalid transition
    Hash256  pre_state_root;      // Valid state root before the transition
    Hash256  claimed_post_root;   // Invalid state root claimed by the sequencer
    Hash256  correct_post_root;   // Correct state root computed by the challenger

    // Merkle proof showing the invalid state element
    std::vector<std::vector<uint8_t>> fraud_proof_nodes;

    // Challenger's signature
    ValidatorSignature challenger_signature;
};

}  // namespace bridge
}  // namespace pantheon
