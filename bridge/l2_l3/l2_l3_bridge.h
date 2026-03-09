#pragma once
// l2_l3_bridge.h — DRACHMA (L2) ↔ OBOLOS (L3) Bridge Interface
//
// Implements the canonical lock-mint / burn-unlock bridge.
// - Lock DRC on DRACHMA → mint wDRC on OBOLOS
// - Burn wDRC on OBOLOS → unlock DRC on DRACHMA

#include "bridge/cross_chain_message.h"
#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace pantheon {
namespace bridge {
namespace l2_l3 {

// Minimum L2 confirmation depth before a bridge unlock or mint is processed.
// Must match configs/*/l2.json: bridge_min_l2_depth.
static constexpr uint32_t kMinL2Confirmations = 10;

// Bridge state for the L2↔L3 bridge.
struct BridgeState {
    std::unordered_set<std::string> processed_nonce_keys;  // "<sender>:<nonce>"
    uint64_t total_locked_drc_base_units{0};
    uint64_t total_minted_wdrc_base_units{0};

    // Trusted validator public keys (x-only 32-byte BIP-340 Schnorr keys).
    // A CrossChainMessage must carry at least min_validator_sigs valid Schnorr
    // signatures from keys in this set before any state change is permitted.
    // Using std::set provides O(log n) membership tests and enforces uniqueness.
    std::set<std::array<uint8_t, 32>> trusted_validator_pubkeys;

    // Minimum number of valid signatures from trusted validators required to
    // accept a CrossChainMessage.  Must be > 0 in production.
    uint32_t min_validator_sigs{1};
};

enum class BridgeResult : uint8_t {
    OK                        = 0,
    ERR_INVALID_PROOF         = 1,
    ERR_REPLAY                = 2,
    ERR_AMOUNT_ZERO           = 3,
    ERR_INVALID_CHAIN         = 4,
    ERR_INSUFFICIENT_CONF     = 5,  // L2 block not deep enough
    ERR_SUPPLY_OVERFLOW       = 6,  // Would exceed max wDRC supply
    ERR_INSUFFICIENT_SIGNATURES = 7, // Validator quorum not met
};

bool VerifyMerkleProof(
    const Hash256& leaf_hash,
    const std::vector<std::vector<uint8_t>>& proof_nodes,
    const Hash256& expected_root);

// Verify the validator signature quorum on a CrossChainMessage.
//
// Returns true iff the number of valid Schnorr signatures from trusted
// validators in message.validator_signatures >= state.min_validator_sigs.
bool VerifyValidatorQuorum(
    const BridgeState& state,
    const CrossChainMessage& message);

// ── L2 side (DRACHMA) ────────────────────────────────────────────────────────

// Record a DRC lock on DRACHMA. Called by the bridge contract when a user
// deposits DRC to be bridged to OBOLOS.
//
// Returns OK on success. The BridgeTransferIntent is serialised and emitted
// as an on-chain event so relayers can pick it up.
BridgeResult RecordDrcLock(
    BridgeState& state,
    const BridgeTransferIntent& intent);

// Verify and execute a wDRC burn proof from OBOLOS.
// Called when the L2 bridge receives a burn message from a relayer.
//
// Checks:
//   1. origin == OBOLOS, destination == DRACHMA
//   2. Merkle proof against the OBOLOS state root
//   3. Nonce not already processed (replay protection)
//   4. L2 block is at least kMinL2Confirmations deep
//
// On success, unlocks DRC to intent.recipient_address.
BridgeResult ProcessWdrcBurnUnlock(
    BridgeState& state,
    const CrossChainMessage& message,
    uint64_t current_l2_height,
    uint64_t lock_l2_height);

// ── L3 side (OBOLOS) ─────────────────────────────────────────────────────────

// Verify and execute a DRC lock proof from DRACHMA.
// Called when the L3 bridge receives a lock message from a relayer.
//
// Checks:
//   1. origin == DRACHMA, destination == OBOLOS
//   2. Merkle proof against the DRACHMA state root
//   3. Nonce not already processed (replay protection)
//   4. ≥ kMinL2Confirmations L2 confirmations
//
// On success, mints wDRC to intent.recipient_address.
BridgeResult ProcessDrcLockMint(
    BridgeState& state,
    const CrossChainMessage& message,
    uint64_t current_l2_height,
    uint64_t lock_l2_height);

// Record a wDRC burn on OBOLOS. Called when a user burns wDRC to recover DRC.
//
// Returns OK on success. The BridgeTransferIntent is emitted on-chain so
// the L2 bridge can process the unlock.
BridgeResult RecordWdrcBurn(
    BridgeState& state,
    const BridgeTransferIntent& intent);

}  // namespace l2_l3
}  // namespace bridge
}  // namespace pantheon
