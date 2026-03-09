#pragma once
// l2_l3_bridge.h — DRACHMA (L2) ↔ OBOLOS (L3) Bridge Interface
//
// Implements the canonical lock-mint / burn-unlock bridge.
// - Lock DRC on DRACHMA → mint wDRC on OBOLOS
// - Burn wDRC on OBOLOS → unlock DRC on DRACHMA

#include "bridge/cross_chain_message.h"
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace pantheon {
namespace bridge {
namespace l2_l3 {

// Bridge state for the L2↔L3 bridge.
struct BridgeState {
    std::unordered_set<std::string> processed_nonce_keys;  // "<sender>:<nonce>"
    uint64_t total_locked_drc_base_units{0};
    uint64_t total_minted_wdrc_base_units{0};
};

enum class BridgeResult : uint8_t {
    OK                = 0,
    ERR_INVALID_PROOF = 1,
    ERR_REPLAY        = 2,
    ERR_AMOUNT_ZERO   = 3,
    ERR_INVALID_CHAIN = 4,
};

bool VerifyMerkleProof(
    const Hash256& leaf_hash,
    const std::vector<std::vector<uint8_t>>& proof_nodes,
    const Hash256& expected_root);

// ── L2 side (DRACHMA) ────────────────────────────────────────────────────────

// Record a DRC lock on DRACHMA.
BridgeResult RecordDrcLock(
    BridgeState& state,
    const BridgeTransferIntent& intent);

// Process a wDRC burn proof from OBOLOS → unlock DRC on DRACHMA.
BridgeResult ProcessWdrcBurnUnlock(
    BridgeState& state,
    const CrossChainMessage& message);

// ── L3 side (OBOLOS) ─────────────────────────────────────────────────────────

// Process a DRC lock proof from DRACHMA → mint wDRC on OBOLOS.
BridgeResult ProcessDrcLockMint(
    BridgeState& state,
    const CrossChainMessage& message);

// Record a wDRC burn on OBOLOS.
BridgeResult RecordWdrcBurn(
    BridgeState& state,
    const BridgeTransferIntent& intent);

}  // namespace l2_l3
}  // namespace bridge
}  // namespace pantheon
