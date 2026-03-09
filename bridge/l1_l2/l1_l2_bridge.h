#pragma once
// l1_l2_bridge.h — TALANTON (L1) ↔ DRACHMA (L2) Bridge Interface
//
// Implements the canonical lock-mint / burn-unlock bridge.
// - Lock TLT on TALANTON → mint wTLT on DRACHMA
// - Burn wTLT on DRACHMA → unlock TLT on TALANTON

#include "bridge/cross_chain_message.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace pantheon {
namespace bridge {
namespace l1_l2 {

// Minimum L1 confirmation depth before a bridge unlock is processed.
// Must match configs/*/l1.json: bridge_unlock_min_l1_depth.
static constexpr uint32_t kMinL1Confirmations = 6;

// Bridge state persisted on each chain side.
struct BridgeState {
    // Nonces already processed — keyed by sender address.
    // Replay protection: a (sender, nonce) pair MUST only be processed once.
    std::unordered_set<std::string> processed_nonce_keys;  // "<sender>:<nonce>"

    // Total TLT locked on L1 (in base units).
    uint64_t total_locked_tlt_base_units{0};

    // Total wTLT minted on L2 (in base units).
    uint64_t total_minted_wtlt_base_units{0};
};

// Result of a bridge operation.
enum class BridgeResult : uint8_t {
    OK                    = 0,
    ERR_INVALID_PROOF     = 1,
    ERR_REPLAY            = 2,       // Nonce already used
    ERR_INSUFFICIENT_CONF = 3,       // L1 block not deep enough
    ERR_AMOUNT_ZERO       = 4,
    ERR_SUPPLY_OVERFLOW   = 5,       // Would exceed max wTLT supply
    ERR_FRAUD_DETECTED    = 6,
    ERR_INVALID_CHAIN     = 7,       // Wrong origin/destination chain ID
};

// Verify a Merkle inclusion proof for a bridge transfer.
//
// proof_nodes: sibling hashes from the leaf to the root (bottom-up).
// Returns true iff SHA256d(leaf ‖ proof_nodes) == expected_root.
bool VerifyMerkleProof(
    const Hash256& leaf_hash,
    const std::vector<std::vector<uint8_t>>& proof_nodes,
    const Hash256& expected_root);

// ── L1 side (TALANTON) ───────────────────────────────────────────────────────

// Record a TLT lock on TALANTON. Called by the bridge contract when a user
// deposits TLT to be bridged to DRACHMA.
//
// Returns OK on success. The BridgeTransferIntent is serialised and emitted
// as an on-chain event so relayers can pick it up.
BridgeResult RecordTltLock(
    BridgeState& state,
    const BridgeTransferIntent& intent);

// Verify and execute a wTLT burn proof from DRACHMA.
// Called when the L1 bridge receives a burn message from a relayer.
//
// Checks:
//   1. origin == DRACHMA, destination == TALANTON
//   2. Merkle proof against the DRACHMA state root
//   3. Nonce not already processed (replay protection)
//   4. L1 block is at least kMinL1Confirmations deep
//
// On success, the bridge unlocks TLT to intent.recipient_address.
BridgeResult ProcessWtltBurnUnlock(
    BridgeState& state,
    const CrossChainMessage& message,
    uint64_t current_l1_height,
    uint64_t lock_l1_height);

// ── L2 side (DRACHMA) ────────────────────────────────────────────────────────

// Verify and execute a TLT lock proof from TALANTON.
// Called when the L2 bridge receives a lock message from a relayer.
//
// Checks:
//   1. origin == TALANTON, destination == DRACHMA
//   2. Merkle proof against the TALANTON state root
//   3. Nonce not already processed (replay protection)
//   4. ≥ kMinL1Confirmations L1 confirmations
//
// On success, mints wTLT to intent.recipient_address.
BridgeResult ProcessTltLockMint(
    BridgeState& state,
    const CrossChainMessage& message,
    uint64_t current_l1_height,
    uint64_t lock_l1_height);

// Record a wTLT burn on DRACHMA. Called when a user burns wTLT to recover TLT.
//
// Returns OK on success. The BridgeTransferIntent is emitted on-chain so
// the L1 bridge can process the unlock.
BridgeResult RecordWtltBurn(
    BridgeState& state,
    const BridgeTransferIntent& intent);

}  // namespace l1_l2
}  // namespace bridge
}  // namespace pantheon
