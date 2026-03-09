// PantheonChain — Adversarial Bridge Attack Tests (Phase C)
//
// Tests every category of bridge attack described in the pre-launch adversarial
// simulation campaign:
//   • replay (same nonce submitted twice)
//   • cross-chain ID confusion (L1→L2 message to L2→L3 bridge)
//   • forged mint without a corresponding lock
//   • insufficient confirmation depth
//   • supply overflow / double-mint
//   • zero-amount dust attacks
//   • wrong-direction messages
//   • invalid Merkle proof rejection
//   • supply invariant (total_minted ≤ total_locked) after full round-trip
//   • L2↔L3 bridge confirmation depth (regression for missing kMinL2Confirmations)
//   • real SHA-256d Merkle proof verification (no stub)

#include "bridge/l1_l2/l1_l2_bridge.h"
#include "bridge/l2_l3/l2_l3_bridge.h"
#include "bridge/cross_chain_message.h"
#include "crypto/sha256.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace pantheon::bridge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a valid CrossChainMessage for L1→L2 (TALANTON → DRACHMA).
// Uses an empty proof list so VerifyMerkleProof passes when
// payload_hash == state_root (single-leaf Merkle tree stub).
static CrossChainMessage make_l1_to_l2_message(uint64_t nonce, uint64_t amount,
                                                uint64_t lock_height)
{
    CrossChainMessage msg;
    msg.origin_chain_id      = ChainId::TALANTON;
    msg.destination_chain_id = ChainId::DRACHMA;
    msg.message_nonce        = nonce;
    msg.block_height         = lock_height;
    msg.timestamp            = 1000000 + lock_height;

    // Encode amount into first 8 bytes of payload (little-endian uint64).
    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i) {
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    }

    // Set payload_hash == state_root so an empty proof list passes (stub behaviour).
    msg.payload_hash.fill(0xAB);
    msg.state_root = msg.payload_hash;  // empty proof: current == leaf_hash == state_root
    // proof is left empty → VerifyMerkleProof returns (leaf_hash == state_root) == true

    return msg;
}

// Build a valid CrossChainMessage for L2→L1 (DRACHMA → TALANTON) burn/unlock.
static CrossChainMessage make_l2_to_l1_message(uint64_t nonce, uint64_t amount,
                                                uint64_t lock_height)
{
    CrossChainMessage msg;
    msg.origin_chain_id      = ChainId::DRACHMA;
    msg.destination_chain_id = ChainId::TALANTON;
    msg.message_nonce        = nonce;
    msg.block_height         = lock_height;
    msg.timestamp            = 2000000 + lock_height;

    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i) {
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    }

    msg.payload_hash.fill(0xCD);
    msg.state_root = msg.payload_hash;

    return msg;
}

// Build a valid CrossChainMessage for L2→L3 (DRACHMA → OBOLOS) lock-mint.
static CrossChainMessage make_l2_to_l3_message(uint64_t nonce, uint64_t amount,
                                                uint64_t lock_height)
{
    CrossChainMessage msg;
    msg.origin_chain_id      = ChainId::DRACHMA;
    msg.destination_chain_id = ChainId::OBOLOS;
    msg.message_nonce        = nonce;
    msg.block_height         = lock_height;
    msg.timestamp            = 3000000 + lock_height;

    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i) {
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    }

    msg.payload_hash.fill(0xEF);
    msg.state_root = msg.payload_hash;

    return msg;
}

// Build a valid CrossChainMessage for L3→L2 (OBOLOS → DRACHMA) burn/unlock.
static CrossChainMessage make_l3_to_l2_message(uint64_t nonce, uint64_t amount,
                                                uint64_t lock_height)
{
    CrossChainMessage msg;
    msg.origin_chain_id      = ChainId::OBOLOS;
    msg.destination_chain_id = ChainId::DRACHMA;
    msg.message_nonce        = nonce;
    msg.block_height         = lock_height;
    msg.timestamp            = 4000000 + lock_height;

    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i) {
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    }

    msg.payload_hash.fill(0x12);
    msg.state_root = msg.payload_hash;

    return msg;
}

// Build a BridgeTransferIntent for L1→L2.
static BridgeTransferIntent make_tlt_lock_intent(uint64_t nonce, uint64_t amount)
{
    BridgeTransferIntent intent;
    intent.origin_chain_id      = ChainId::TALANTON;
    intent.destination_chain_id = ChainId::DRACHMA;
    intent.sender_address       = "alice";
    intent.recipient_address    = "alice_on_l2";
    intent.token_symbol         = "TLT";
    intent.wrapped_token_symbol = "wTLT";
    intent.amount_base_units    = amount;
    intent.nonce                = nonce;
    intent.lock_block_height    = 100;
    return intent;
}

// Build a BridgeTransferIntent for L2→L1 wTLT burn.
static BridgeTransferIntent make_wtlt_burn_intent(uint64_t nonce, uint64_t amount)
{
    BridgeTransferIntent intent;
    intent.origin_chain_id      = ChainId::DRACHMA;
    intent.destination_chain_id = ChainId::TALANTON;
    intent.sender_address       = "alice_on_l2";
    intent.recipient_address    = "alice";
    intent.token_symbol         = "wTLT";
    intent.wrapped_token_symbol = "TLT";
    intent.amount_base_units    = amount;
    intent.nonce                = nonce;
    intent.lock_block_height    = 200;
    return intent;
}

// Build a BridgeTransferIntent for L2→L3.
static BridgeTransferIntent make_drc_lock_intent(uint64_t nonce, uint64_t amount)
{
    BridgeTransferIntent intent;
    intent.origin_chain_id      = ChainId::DRACHMA;
    intent.destination_chain_id = ChainId::OBOLOS;
    intent.sender_address       = "bob";
    intent.recipient_address    = "bob_on_l3";
    intent.token_symbol         = "DRC";
    intent.wrapped_token_symbol = "wDRC";
    intent.amount_base_units    = amount;
    intent.nonce                = nonce;
    intent.lock_block_height    = 300;
    return intent;
}

// Build a BridgeTransferIntent for L3→L2 wDRC burn.
static BridgeTransferIntent make_wdrc_burn_intent(uint64_t nonce, uint64_t amount)
{
    BridgeTransferIntent intent;
    intent.origin_chain_id      = ChainId::OBOLOS;
    intent.destination_chain_id = ChainId::DRACHMA;
    intent.sender_address       = "bob_on_l3";
    intent.recipient_address    = "bob";
    intent.token_symbol         = "wDRC";
    intent.wrapped_token_symbol = "DRC";
    intent.amount_base_units    = amount;
    intent.nonce                = nonce;
    intent.lock_block_height    = 400;
    return intent;
}

// ---------------------------------------------------------------------------
// Phase C Tests — L1↔L2 Bridge
// ---------------------------------------------------------------------------

void test_l1_l2_normal_round_trip() {
    std::cout << "[l1_l2] Normal lock → mint → burn → unlock round-trip" << std::endl;

    l1_l2::BridgeState l1_state, l2_state;
    const uint64_t AMOUNT = 1000;

    // Lock TLT on L1.
    auto intent = make_tlt_lock_intent(1, AMOUNT);
    assert(l1_l2::RecordTltLock(l1_state, intent) == l1_l2::BridgeResult::OK);
    assert(l1_state.total_locked_tlt_base_units == AMOUNT);

    // L2 bridge state is informed of the locked amount by the relayer (mirrors L1 lock).
    l2_state.total_locked_tlt_base_units = AMOUNT;

    // Mint wTLT on L2 (with enough L1 confirmations).
    auto lock_msg = make_l1_to_l2_message(1, AMOUNT, 100);
    assert(l1_l2::ProcessTltLockMint(l2_state, lock_msg, 110, 100) == l1_l2::BridgeResult::OK);
    assert(l2_state.total_minted_wtlt_base_units == AMOUNT);

    // Burn wTLT on L2.
    auto burn_intent = make_wtlt_burn_intent(1, AMOUNT);
    assert(l1_l2::RecordWtltBurn(l2_state, burn_intent) == l1_l2::BridgeResult::OK);
    assert(l2_state.total_minted_wtlt_base_units == 0);

    // Unlock TLT on L1.
    auto burn_msg = make_l2_to_l1_message(1, AMOUNT, 200);
    assert(l1_l2::ProcessWtltBurnUnlock(l1_state, burn_msg, 210, 200) == l1_l2::BridgeResult::OK);
    assert(l1_state.total_locked_tlt_base_units == 0);

    // Supply invariant: both locked and minted return to zero.
    assert(l1_state.total_locked_tlt_base_units == 0);
    assert(l2_state.total_minted_wtlt_base_units == 0);

    std::cout << "  ✓ Normal round-trip passed; supply conserved." << std::endl;
}

void test_l1_l2_replay_attack() {
    std::cout << "[l1_l2] Replay attack: same nonce processed twice" << std::endl;

    l1_l2::BridgeState state;
    const uint64_t AMOUNT = 500;

    // First submission succeeds.
    auto msg = make_l1_to_l2_message(42, AMOUNT, 100);
    state.total_locked_tlt_base_units = AMOUNT;
    assert(l1_l2::ProcessTltLockMint(state, msg, 110, 100) == l1_l2::BridgeResult::OK);
    assert(state.total_minted_wtlt_base_units == AMOUNT);

    // Second submission with SAME nonce must be rejected.
    auto result = l1_l2::ProcessTltLockMint(state, msg, 110, 100);
    assert(result == l1_l2::BridgeResult::ERR_REPLAY);
    // Supply must not have increased.
    assert(state.total_minted_wtlt_base_units == AMOUNT);

    std::cout << "  ✓ Replay correctly rejected." << std::endl;
}

void test_l1_l2_insufficient_confirmations() {
    std::cout << "[l1_l2] Insufficient confirmation depth" << std::endl;

    l1_l2::BridgeState state;
    state.total_locked_tlt_base_units = 1000;

    auto msg = make_l1_to_l2_message(1, 1000, 100);

    // Only 3 confirmations (need 6).
    auto result = l1_l2::ProcessTltLockMint(state, msg, 103, 100);
    assert(result == l1_l2::BridgeResult::ERR_INSUFFICIENT_CONF);
    assert(state.total_minted_wtlt_base_units == 0);

    // Exactly at the boundary (5 confirmations) — still insufficient.
    result = l1_l2::ProcessTltLockMint(state, msg, 105, 100);
    assert(result == l1_l2::BridgeResult::ERR_INSUFFICIENT_CONF);

    // At exactly 6 confirmations — should pass.
    result = l1_l2::ProcessTltLockMint(state, msg, 106, 100);
    assert(result == l1_l2::BridgeResult::OK);

    std::cout << "  ✓ Confirmation depth enforced correctly." << std::endl;
}

void test_l1_l2_wrong_chain_ids() {
    std::cout << "[l1_l2] Wrong chain ID on lock intent" << std::endl;

    l1_l2::BridgeState state;

    // Intent with wrong origin (DRACHMA instead of TALANTON).
    BridgeTransferIntent bad_intent = make_tlt_lock_intent(1, 100);
    bad_intent.origin_chain_id = ChainId::DRACHMA;
    assert(l1_l2::RecordTltLock(state, bad_intent) == l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // Intent with wrong destination (OBOLOS instead of DRACHMA).
    bad_intent = make_tlt_lock_intent(1, 100);
    bad_intent.destination_chain_id = ChainId::OBOLOS;
    assert(l1_l2::RecordTltLock(state, bad_intent) == l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // CrossChainMessage with wrong origin (OBOLOS instead of TALANTON).
    auto bad_msg = make_l1_to_l2_message(1, 100, 100);
    bad_msg.origin_chain_id = ChainId::OBOLOS;
    state.total_locked_tlt_base_units = 100;
    assert(l1_l2::ProcessTltLockMint(state, bad_msg, 110, 100) == l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // Direct L1→L3 hop (bypassing L2) must be rejected by the L1↔L2 bridge.
    bad_msg = make_l1_to_l2_message(2, 100, 100);
    bad_msg.destination_chain_id = ChainId::OBOLOS;
    assert(l1_l2::ProcessTltLockMint(state, bad_msg, 110, 100) == l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ Wrong chain IDs rejected." << std::endl;
}

void test_l1_l2_zero_amount() {
    std::cout << "[l1_l2] Zero-amount (dust) attack" << std::endl;

    l1_l2::BridgeState state;

    // Zero amount lock intent must be rejected.
    auto intent = make_tlt_lock_intent(1, 0);
    assert(l1_l2::RecordTltLock(state, intent) == l1_l2::BridgeResult::ERR_AMOUNT_ZERO);
    assert(state.total_locked_tlt_base_units == 0);

    // Zero amount burn intent must be rejected.
    auto burn = make_wtlt_burn_intent(1, 0);
    assert(l1_l2::RecordWtltBurn(state, burn) == l1_l2::BridgeResult::ERR_AMOUNT_ZERO);

    std::cout << "  ✓ Zero-amount attacks rejected." << std::endl;
}

void test_l1_l2_supply_overflow() {
    std::cout << "[l1_l2] Supply overflow protection" << std::endl;

    l1_l2::BridgeState state;

    // Lock a normal amount first.
    auto intent1 = make_tlt_lock_intent(1, 1000);
    assert(l1_l2::RecordTltLock(state, intent1) == l1_l2::BridgeResult::OK);

    // Lock near uint64_max — should trigger overflow guard.
    auto intent2 = make_tlt_lock_intent(2, UINT64_MAX - 500);
    auto result = l1_l2::RecordTltLock(state, intent2);
    assert(result == l1_l2::BridgeResult::ERR_SUPPLY_OVERFLOW);
    // Supply must remain unchanged after the rejected lock.
    assert(state.total_locked_tlt_base_units == 1000);

    // Mint more than locked — supply overflow guard.
    state.total_locked_tlt_base_units = 500;
    auto msg = make_l1_to_l2_message(1, 600, 100);  // 600 > 500 locked
    result = l1_l2::ProcessTltLockMint(state, msg, 110, 100);
    assert(result == l1_l2::BridgeResult::ERR_SUPPLY_OVERFLOW);
    assert(state.total_minted_wtlt_base_units == 0);

    std::cout << "  ✓ Supply overflow protection works." << std::endl;
}

void test_l1_l2_invalid_proof() {
    std::cout << "[l1_l2] Invalid Merkle proof rejection" << std::endl;

    l1_l2::BridgeState state;
    state.total_locked_tlt_base_units = 1000;

    // Construct message where state_root ≠ payload_hash and no proof nodes.
    // VerifyMerkleProof: current = payload_hash, returns (payload_hash == state_root).
    CrossChainMessage msg = make_l1_to_l2_message(1, 1000, 100);
    msg.payload_hash.fill(0x01);
    msg.state_root.fill(0x02);  // Mismatch → proof fails
    msg.proof.clear();

    auto result = l1_l2::ProcessTltLockMint(state, msg, 110, 100);
    assert(result == l1_l2::BridgeResult::ERR_INVALID_PROOF);
    assert(state.total_minted_wtlt_base_units == 0);

    std::cout << "  ✓ Invalid Merkle proof correctly rejected." << std::endl;
}

void test_l1_l2_cross_chain_replay_to_l2l3() {
    std::cout << "[l1_l2→l2_l3] Cross-chain replay: L1→L2 message submitted to L2→L3 bridge" << std::endl;

    l2_l3::BridgeState l3_state;
    l3_state.total_locked_drc_base_units = 1000;

    // A valid L1→L2 message (TALANTON → DRACHMA) submitted to the L2→L3 bridge.
    CrossChainMessage l1_to_l2_msg = make_l1_to_l2_message(1, 1000, 100);

    // ProcessDrcLockMint expects origin=DRACHMA, destination=OBOLOS.
    auto result = l2_l3::ProcessDrcLockMint(l3_state, l1_to_l2_msg, 110, 100);
    assert(result == l2_l3::BridgeResult::ERR_INVALID_CHAIN);
    assert(l3_state.total_minted_wdrc_base_units == 0);

    std::cout << "  ✓ Cross-chain replay to L2↔L3 bridge correctly rejected." << std::endl;
}

void test_l1_l2_burn_without_lock() {
    std::cout << "[l1_l2] Burn-without-lock: wTLT burn before any lock recorded" << std::endl;

    l1_l2::BridgeState l1_state;

    // No lock ever recorded on L1 (total_locked == 0).
    // A burn unlock message arrives — should succeed in the check but unlock 0 due to guard.
    auto msg = make_l2_to_l1_message(1, 1000, 200);
    auto result = l1_l2::ProcessWtltBurnUnlock(l1_state, msg, 210, 200);
    // The proof passes (payload_hash == state_root, empty proof), replay check passes,
    // but total_locked is 0 so the amount-decrement guard prevents going negative.
    assert(result == l1_l2::BridgeResult::OK);
    assert(l1_state.total_locked_tlt_base_units == 0);  // No underflow

    std::cout << "  ✓ Burn-without-lock does not produce negative supply." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase C Tests — L2↔L3 Bridge
// ---------------------------------------------------------------------------

void test_l2_l3_normal_round_trip() {
    std::cout << "[l2_l3] Normal lock → mint → burn → unlock round-trip" << std::endl;

    l2_l3::BridgeState l2_state, l3_state;
    const uint64_t AMOUNT = 2000;

    // Lock DRC on L2.
    auto intent = make_drc_lock_intent(1, AMOUNT);
    assert(l2_l3::RecordDrcLock(l2_state, intent) == l2_l3::BridgeResult::OK);
    assert(l2_state.total_locked_drc_base_units == AMOUNT);

    // Mint wDRC on L3.
    auto lock_msg = make_l2_to_l3_message(1, AMOUNT, 300);
    l3_state.total_locked_drc_base_units = AMOUNT;
    assert(l2_l3::ProcessDrcLockMint(l3_state, lock_msg, 315, 300) == l2_l3::BridgeResult::OK);
    assert(l3_state.total_minted_wdrc_base_units == AMOUNT);

    // Burn wDRC on L3.
    auto burn_intent = make_wdrc_burn_intent(1, AMOUNT);
    assert(l2_l3::RecordWdrcBurn(l3_state, burn_intent) == l2_l3::BridgeResult::OK);
    assert(l3_state.total_minted_wdrc_base_units == 0);

    // Unlock DRC on L2.
    auto burn_msg = make_l3_to_l2_message(1, AMOUNT, 400);
    assert(l2_l3::ProcessWdrcBurnUnlock(l2_state, burn_msg, 415, 400) == l2_l3::BridgeResult::OK);
    assert(l2_state.total_locked_drc_base_units == 0);

    std::cout << "  ✓ L2↔L3 round-trip passed; supply conserved." << std::endl;
}

void test_l2_l3_insufficient_confirmations() {
    std::cout << "[l2_l3] Missing confirmation depth (regression: kMinL2Confirmations)" << std::endl;

    l2_l3::BridgeState state;
    state.total_locked_drc_base_units = 1000;

    auto msg = make_l2_to_l3_message(1, 1000, 300);

    // Only 5 confirmations (need 10).
    auto result = l2_l3::ProcessDrcLockMint(state, msg, 305, 300);
    assert(result == l2_l3::BridgeResult::ERR_INSUFFICIENT_CONF);
    assert(state.total_minted_wdrc_base_units == 0);

    // 9 confirmations — still insufficient.
    result = l2_l3::ProcessDrcLockMint(state, msg, 309, 300);
    assert(result == l2_l3::BridgeResult::ERR_INSUFFICIENT_CONF);

    // Exactly 10 confirmations — should pass.
    result = l2_l3::ProcessDrcLockMint(state, msg, 310, 300);
    assert(result == l2_l3::BridgeResult::OK);

    std::cout << "  ✓ L2↔L3 confirmation depth enforced (kMinL2Confirmations=10)." << std::endl;
}

void test_l2_l3_burn_unlock_insufficient_conf() {
    std::cout << "[l2_l3] Burn-unlock insufficient confirmation depth" << std::endl;

    l2_l3::BridgeState state;
    state.total_locked_drc_base_units = 1000;

    auto msg = make_l3_to_l2_message(1, 1000, 400);

    // 9 confirmations — insufficient.
    auto result = l2_l3::ProcessWdrcBurnUnlock(state, msg, 409, 400);
    assert(result == l2_l3::BridgeResult::ERR_INSUFFICIENT_CONF);

    // Exactly 10 confirmations — should pass.
    result = l2_l3::ProcessWdrcBurnUnlock(state, msg, 410, 400);
    assert(result == l2_l3::BridgeResult::OK);

    std::cout << "  ✓ L2↔L3 unlock confirmation depth enforced." << std::endl;
}

void test_l2_l3_replay_attack() {
    std::cout << "[l2_l3] Replay attack: same nonce processed twice" << std::endl;

    l2_l3::BridgeState state;
    state.total_locked_drc_base_units = 2000;

    auto msg = make_l2_to_l3_message(7, 1000, 300);
    assert(l2_l3::ProcessDrcLockMint(state, msg, 315, 300) == l2_l3::BridgeResult::OK);
    assert(state.total_minted_wdrc_base_units == 1000);

    // Second submission with same nonce must be rejected.
    auto result = l2_l3::ProcessDrcLockMint(state, msg, 315, 300);
    assert(result == l2_l3::BridgeResult::ERR_REPLAY);
    assert(state.total_minted_wdrc_base_units == 1000);  // Unchanged

    std::cout << "  ✓ L2↔L3 replay correctly rejected." << std::endl;
}

void test_l2_l3_supply_overflow() {
    std::cout << "[l2_l3] Supply overflow: mint > locked" << std::endl;

    l2_l3::BridgeState state;
    state.total_locked_drc_base_units = 500;

    // Attempt to mint 600 when only 500 is locked.
    auto msg = make_l2_to_l3_message(1, 600, 300);
    auto result = l2_l3::ProcessDrcLockMint(state, msg, 315, 300);
    assert(result == l2_l3::BridgeResult::ERR_SUPPLY_OVERFLOW);
    assert(state.total_minted_wdrc_base_units == 0);

    // Lock near uint64_max — should trigger overflow guard.
    l2_l3::BridgeState s2;
    auto intent = make_drc_lock_intent(1, 1000);
    assert(l2_l3::RecordDrcLock(s2, intent) == l2_l3::BridgeResult::OK);

    auto intent2 = make_drc_lock_intent(2, UINT64_MAX - 500);
    assert(l2_l3::RecordDrcLock(s2, intent2) == l2_l3::BridgeResult::ERR_SUPPLY_OVERFLOW);
    assert(s2.total_locked_drc_base_units == 1000);

    std::cout << "  ✓ L2↔L3 supply overflow protection works." << std::endl;
}

void test_l2_l3_wrong_chain_ids() {
    std::cout << "[l2_l3] Wrong chain ID on L2↔L3 bridge" << std::endl;

    l2_l3::BridgeState state;
    state.total_locked_drc_base_units = 1000;

    // L2→L1 message (DRACHMA→TALANTON) submitted to L3 mint function.
    CrossChainMessage l2_to_l1_msg = make_l2_to_l1_message(1, 1000, 200);
    auto result = l2_l3::ProcessDrcLockMint(state, l2_to_l1_msg, 215, 200);
    assert(result == l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    // L1→L2 message (TALANTON→DRACHMA) submitted to L2 unlock function.
    CrossChainMessage l1_to_l2_msg = make_l1_to_l2_message(1, 1000, 100);
    result = l2_l3::ProcessWdrcBurnUnlock(state, l1_to_l2_msg, 115, 100);
    assert(result == l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ L2↔L3 wrong chain ID rejected." << std::endl;
}

void test_l2_l3_zero_amount() {
    std::cout << "[l2_l3] Zero-amount (dust) attack on L2↔L3" << std::endl;

    l2_l3::BridgeState state;

    auto intent = make_drc_lock_intent(1, 0);
    assert(l2_l3::RecordDrcLock(state, intent) == l2_l3::BridgeResult::ERR_AMOUNT_ZERO);

    auto burn = make_wdrc_burn_intent(1, 0);
    assert(l2_l3::RecordWdrcBurn(state, burn) == l2_l3::BridgeResult::ERR_AMOUNT_ZERO);

    std::cout << "  ✓ L2↔L3 zero-amount attacks rejected." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase K Invariant: No relayer can create value
// ---------------------------------------------------------------------------

void test_supply_invariant_across_chains() {
    std::cout << "[invariant] Total minted never exceeds total locked across bridge lifecycle" << std::endl;

    // Simulate a sequence of operations and verify supply invariant after each step.
    l1_l2::BridgeState l1_side, l2_side;

    const uint64_t A1 = 3000, A2 = 1500;

    // Lock two tranches on L1.
    assert(l1_l2::RecordTltLock(l1_side, make_tlt_lock_intent(1, A1)) == l1_l2::BridgeResult::OK);
    assert(l1_l2::RecordTltLock(l1_side, make_tlt_lock_intent(2, A2)) == l1_l2::BridgeResult::OK);
    assert(l1_side.total_locked_tlt_base_units == A1 + A2);

    // Mint tranche 1 on L2.
    l2_side.total_locked_tlt_base_units = A1 + A2;
    auto m1 = make_l1_to_l2_message(1, A1, 100);
    assert(l1_l2::ProcessTltLockMint(l2_side, m1, 110, 100) == l1_l2::BridgeResult::OK);
    assert(l2_side.total_minted_wtlt_base_units <= l2_side.total_locked_tlt_base_units);

    // Mint tranche 2 on L2.
    auto m2 = make_l1_to_l2_message(2, A2, 105);
    assert(l1_l2::ProcessTltLockMint(l2_side, m2, 115, 105) == l1_l2::BridgeResult::OK);
    assert(l2_side.total_minted_wtlt_base_units == A1 + A2);
    assert(l2_side.total_minted_wtlt_base_units <= l2_side.total_locked_tlt_base_units);

    // Burn tranche 1 on L2.
    assert(l1_l2::RecordWtltBurn(l2_side, make_wtlt_burn_intent(1, A1)) == l1_l2::BridgeResult::OK);
    assert(l2_side.total_minted_wtlt_base_units == A2);

    // Unlock tranche 1 on L1.
    auto u1 = make_l2_to_l1_message(1, A1, 200);
    assert(l1_l2::ProcessWtltBurnUnlock(l1_side, u1, 210, 200) == l1_l2::BridgeResult::OK);
    assert(l1_side.total_locked_tlt_base_units == A2);

    // Final state: A2 still locked on L1, A2 still minted on L2.
    assert(l1_side.total_locked_tlt_base_units == A2);
    assert(l2_side.total_minted_wtlt_base_units == A2);
    assert(l2_side.total_minted_wtlt_base_units <= l2_side.total_locked_tlt_base_units);

    std::cout << "  ✓ Supply invariant (minted ≤ locked) maintained across full lifecycle." << std::endl;
}

// ---------------------------------------------------------------------------
// Real SHA-256d Merkle proof correctness (stub-gone regression test)
// ---------------------------------------------------------------------------

// Compute SHA-256d of combined left ‖ right, mirroring VerifyMerkleProof.
static Hash256 sha256d_node(const Hash256& left, const Hash256& right)
{
    std::vector<uint8_t> combined(left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    return parthenon::crypto::SHA256d::Hash256d(combined.data(), combined.size());
}

void test_verify_merkle_proof_real_sha256d() {
    std::cout << "[merkle] VerifyMerkleProof uses real SHA-256d (no stub)" << std::endl;

    // Build a 2-leaf Merkle tree:
    //   leaf0 = 0x00 * 32   leaf1 = 0x01 * 32
    //   root  = SHA256d(leaf0 ‖ leaf1)
    //
    // VerifyMerkleProof always forms: combined = current ‖ sibling.
    // Proof for leaf0 (left child): sibling = leaf1,
    //   hash step: SHA256d(leaf0 ‖ leaf1) == root  ✓
    Hash256 leaf0{}; leaf0.fill(0x00);
    Hash256 leaf1{}; leaf1.fill(0x01);
    const Hash256 root = sha256d_node(leaf0, leaf1);

    // Correct proof for leaf0.
    assert(l1_l2::VerifyMerkleProof(leaf0, {std::vector<uint8_t>(leaf1.begin(), leaf1.end())}, root));

    // Wrong root must fail.
    Hash256 wrong_root{}; wrong_root.fill(0xFF);
    assert(!l1_l2::VerifyMerkleProof(leaf0,
        {std::vector<uint8_t>(leaf1.begin(), leaf1.end())}, wrong_root));

    // --- stub-gone sanity: if the stub were still present it would return
    // Hash256{} for every node, so the computed root of any single-sibling proof
    // would always be Hash256{} (all zeros).  Reuse leaf0 (also all zeros) to
    // verify the real root is non-zero.
    assert(root != leaf0);  // Would fail only if sha256d_stub were still active

    // Test L2↔L3 bridge VerifyMerkleProof independently (same implementation).
    assert(l2_l3::VerifyMerkleProof(leaf0, {std::vector<uint8_t>(leaf1.begin(), leaf1.end())}, root));
    assert(!l2_l3::VerifyMerkleProof(leaf0,
        {std::vector<uint8_t>(leaf1.begin(), leaf1.end())}, wrong_root));

    // Build a 3-level, 4-leaf tree and prove leaf0:
    //   leaf0..3 = 0x00..0x03 * 32
    //   parent01 = SHA256d(leaf0 ‖ leaf1)
    //   parent23 = SHA256d(leaf2 ‖ leaf3)
    //   root4    = SHA256d(parent01 ‖ parent23)
    // Proof for leaf0: [leaf1, parent23]
    //   step 1: SHA256d(leaf0   ‖ leaf1)    = parent01
    //   step 2: SHA256d(parent01 ‖ parent23) = root4  ✓
    Hash256 leaf2{}; leaf2.fill(0x02);
    Hash256 leaf3{}; leaf3.fill(0x03);
    const Hash256 parent01 = sha256d_node(leaf0, leaf1);
    const Hash256 parent23 = sha256d_node(leaf2, leaf3);
    const Hash256 root4    = sha256d_node(parent01, parent23);

    std::vector<std::vector<uint8_t>> proof_leaf0 = {
        std::vector<uint8_t>(leaf1.begin(),    leaf1.end()),
        std::vector<uint8_t>(parent23.begin(), parent23.end()),
    };
    assert(l1_l2::VerifyMerkleProof(leaf0, proof_leaf0, root4));

    // Tampered sibling must fail.
    Hash256 bad_sibling{}; bad_sibling.fill(0xAA);
    std::vector<std::vector<uint8_t>> bad_proof = {
        std::vector<uint8_t>(bad_sibling.begin(), bad_sibling.end()),
        std::vector<uint8_t>(parent23.begin(),    parent23.end()),
    };
    assert(!l1_l2::VerifyMerkleProof(leaf0, bad_proof, root4));

    // Truncated proof (missing one level) must fail.
    std::vector<std::vector<uint8_t>> short_proof = {
        std::vector<uint8_t>(leaf1.begin(), leaf1.end()),
        // parent23 omitted → stops at parent01, which ≠ root4
    };
    assert(!l1_l2::VerifyMerkleProof(leaf0, short_proof, root4));

    std::cout << "  ✓ VerifyMerkleProof uses real SHA-256d; stub is gone." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== PantheonChain Adversarial Bridge Tests (Phase C) ===" << std::endl;
    std::cout << std::endl;

    // L1↔L2 bridge
    test_l1_l2_normal_round_trip();
    test_l1_l2_replay_attack();
    test_l1_l2_insufficient_confirmations();
    test_l1_l2_wrong_chain_ids();
    test_l1_l2_zero_amount();
    test_l1_l2_supply_overflow();
    test_l1_l2_invalid_proof();
    test_l1_l2_cross_chain_replay_to_l2l3();
    test_l1_l2_burn_without_lock();

    // L2↔L3 bridge
    test_l2_l3_normal_round_trip();
    test_l2_l3_insufficient_confirmations();
    test_l2_l3_burn_unlock_insufficient_conf();
    test_l2_l3_replay_attack();
    test_l2_l3_supply_overflow();
    test_l2_l3_wrong_chain_ids();
    test_l2_l3_zero_amount();

    // Cross-layer invariants
    test_supply_invariant_across_chains();

    // Real SHA-256d Merkle proof correctness (stub-gone regression)
    test_verify_merkle_proof_real_sha256d();

    std::cout << std::endl;
    std::cout << "✓ All adversarial bridge tests passed." << std::endl;
    return 0;
}
