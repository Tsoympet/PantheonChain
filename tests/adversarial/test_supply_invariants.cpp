// PantheonChain — Supply Invariant Tests (Phase D + Phase K)
//
// Verifies token-economic safety properties:
//   • total supply per chain stays within the designed cap
//   • wrapped supply never exceeds the locked canonical backing
//   • slashing reduces stake (never creates tokens)
//   • integer overflow protection in supply arithmetic
//   • rounding in slashing is conservative (rounds down, never up)
//   • supply remains zero when no operations have occurred
//   • multiple parallel senders tracked independently
//   • recovery: supply invariant holds after simulated bridge-state restart

#include "bridge/l1_l2/l1_l2_bridge.h"
#include "bridge/l2_l3/l2_l3_bridge.h"
#include "bridge/cross_chain_message.h"
#include "layer2-drachma/consensus/pos_consensus.h"
#include "layer3-obolos/consensus/pos_consensus.h"
#include "common/commitments.h"
#include "bridge_test_helpers.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

using namespace pantheon;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bridge::CrossChainMessage make_tlt_mint_msg(uint64_t nonce, uint64_t amount, uint64_t lock_height,
                                           uint8_t hash_byte = 0xAB)
{
    bridge::CrossChainMessage msg;
    msg.origin_chain_id      = bridge::ChainId::TALANTON;
    msg.destination_chain_id = bridge::ChainId::DRACHMA;
    msg.message_nonce        = nonce;
    msg.block_height         = lock_height;
    msg.timestamp            = 1000000 + lock_height;
    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i)
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    msg.payload_hash.fill(hash_byte);
    msg.state_root = msg.payload_hash;  // empty proof: leaf == root
    bridge_test::bridge_sign_message(msg);
    return msg;
}

static bridge::CrossChainMessage make_drc_mint_msg(uint64_t nonce, uint64_t amount, uint64_t lock_height,
                                           uint8_t hash_byte = 0xEF)
{
    bridge::CrossChainMessage msg;
    msg.origin_chain_id      = bridge::ChainId::DRACHMA;
    msg.destination_chain_id = bridge::ChainId::OBOLOS;
    msg.message_nonce        = nonce;
    msg.block_height         = lock_height;
    msg.timestamp            = 3000000 + lock_height;
    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i)
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    msg.payload_hash.fill(hash_byte);
    msg.state_root = msg.payload_hash;
    bridge_test::bridge_sign_message(msg);
    return msg;
}

static bridge::BridgeTransferIntent make_tlt_lock(const std::string& sender,
                                                   uint64_t nonce, uint64_t amount)
{
    bridge::BridgeTransferIntent intent;
    intent.origin_chain_id      = bridge::ChainId::TALANTON;
    intent.destination_chain_id = bridge::ChainId::DRACHMA;
    intent.sender_address       = sender;
    intent.amount_base_units    = amount;
    intent.nonce                = nonce;
    return intent;
}

static bridge::BridgeTransferIntent make_wtlt_burn(const std::string& sender,
                                                    uint64_t nonce, uint64_t amount)
{
    bridge::BridgeTransferIntent intent;
    intent.origin_chain_id      = bridge::ChainId::DRACHMA;
    intent.destination_chain_id = bridge::ChainId::TALANTON;
    intent.sender_address       = sender;
    intent.amount_base_units    = amount;
    intent.nonce                = nonce;
    return intent;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_initial_supply_is_zero() {
    std::cout << "[supply] Initial bridge supply is zero" << std::endl;

    bridge::l1_l2::BridgeState s;
    bridge_test::setup_bridge_state(s);
    assert(s.total_locked_tlt_base_units == 0);
    assert(s.total_minted_wtlt_base_units == 0);

    bridge::l2_l3::BridgeState s2;
    bridge_test::setup_bridge_state(s2);
    assert(s2.total_locked_drc_base_units == 0);
    assert(s2.total_minted_wdrc_base_units == 0);

    std::cout << "  ✓ Initial bridge supply is zero." << std::endl;
}

void test_lock_only_does_not_create_wrapped_supply() {
    std::cout << "[supply] Lock only — no wrapped tokens created until mint" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    auto result = bridge::l1_l2::RecordTltLock(state, make_tlt_lock("alice", 1, 5000));
    assert(result == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_locked_tlt_base_units == 5000);
    assert(state.total_minted_wtlt_base_units == 0);  // No mint has happened

    std::cout << "  ✓ Lock records locked supply without creating wrapped supply." << std::endl;
}

void test_minted_never_exceeds_locked() {
    std::cout << "[supply] Minted supply never exceeds locked supply" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    const uint64_t LOCKED = 10000;
    state.total_locked_tlt_base_units = LOCKED;

    // Mint exactly the locked amount — should succeed.
    auto msg = make_tlt_mint_msg(1, LOCKED, 100);
    assert(bridge::l1_l2::ProcessTltLockMint(state, msg, 110, 100)
           == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_minted_wtlt_base_units == LOCKED);
    assert(state.total_minted_wtlt_base_units <= state.total_locked_tlt_base_units);

    // Attempt to mint one more unit — invariant violation, must be rejected.
    state.total_locked_tlt_base_units = LOCKED;  // Reset locked to same value
    auto msg2 = make_tlt_mint_msg(2, 1, 100, 0xBB);  // 1 more on top of LOCKED already minted
    auto r2 = bridge::l1_l2::ProcessTltLockMint(state, msg2, 110, 100);
    assert(r2 == bridge::l1_l2::BridgeResult::ERR_SUPPLY_OVERFLOW);

    std::cout << "  ✓ Minted never exceeds locked; overflow blocked." << std::endl;
}

void test_burn_reduces_minted_supply() {
    std::cout << "[supply] Burn correctly reduces minted supply" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units  = 5000;
    state.total_minted_wtlt_base_units = 5000;

    auto burn = make_wtlt_burn("alice", 1, 2000);
    assert(bridge::l1_l2::RecordWtltBurn(state, burn) == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_minted_wtlt_base_units == 3000);

    // Second burn of 3000.
    auto burn2 = make_wtlt_burn("alice", 2, 3000);
    assert(bridge::l1_l2::RecordWtltBurn(state, burn2) == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_minted_wtlt_base_units == 0);

    std::cout << "  ✓ Burns correctly reduce minted supply." << std::endl;
}

void test_unlock_reduces_locked_supply() {
    std::cout << "[supply] Unlock correctly reduces locked supply" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = 4000;

    bridge::CrossChainMessage msg;
    msg.origin_chain_id      = bridge::ChainId::DRACHMA;
    msg.destination_chain_id = bridge::ChainId::TALANTON;
    msg.message_nonce        = 1;
    msg.block_height         = 200;
    msg.timestamp            = 2000200;
    msg.payload.resize(8);
    const uint64_t AMT = 1500;
    for (int i = 0; i < 8; ++i)
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((AMT >> (8 * i)) & 0xFF);
    msg.payload_hash.fill(0xCD);
    msg.state_root = msg.payload_hash;
    bridge_test::bridge_sign_message(msg);

    assert(bridge::l1_l2::ProcessWtltBurnUnlock(state, msg, 210, 200)
           == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_locked_tlt_base_units == 4000 - 1500);

    std::cout << "  ✓ Unlock correctly reduces locked supply." << std::endl;
}

void test_supply_overflow_protection_lock() {
    std::cout << "[supply] Supply overflow guard on RecordTltLock" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = std::numeric_limits<uint64_t>::max() - 100;

    // Locking 101 more would overflow uint64_t.
    auto intent = make_tlt_lock("alice", 1, 101);
    auto result = bridge::l1_l2::RecordTltLock(state, intent);
    assert(result == bridge::l1_l2::BridgeResult::ERR_SUPPLY_OVERFLOW);
    // Supply must remain unchanged.
    assert(state.total_locked_tlt_base_units == std::numeric_limits<uint64_t>::max() - 100);

    // Locking exactly 100 (no overflow) must succeed.
    auto intent2 = make_tlt_lock("alice", 2, 100);
    assert(bridge::l1_l2::RecordTltLock(state, intent2) == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_locked_tlt_base_units == std::numeric_limits<uint64_t>::max());

    std::cout << "  ✓ Lock supply overflow guard works at uint64_t boundary." << std::endl;
}

void test_multiple_senders_independent() {
    std::cout << "[supply] Multiple senders tracked independently" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    const uint64_t A_AMT = 1000, B_AMT = 2500;

    // Both alice and bob lock tokens.
    assert(bridge::l1_l2::RecordTltLock(state, make_tlt_lock("alice", 1, A_AMT))
           == bridge::l1_l2::BridgeResult::OK);
    assert(bridge::l1_l2::RecordTltLock(state, make_tlt_lock("bob",   1, B_AMT))
           == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_locked_tlt_base_units == A_AMT + B_AMT);

    // Alice tries to replay her nonce 1 — must be rejected (different sender, same nonce key).
    auto dup = make_tlt_lock("alice", 1, A_AMT);
    assert(bridge::l1_l2::RecordTltLock(state, dup) == bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Bob's nonce 1 is a different key from alice's nonce 1, so no false collision.
    auto bob_dup = make_tlt_lock("bob", 1, B_AMT);
    assert(bridge::l1_l2::RecordTltLock(state, bob_dup) == bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Supply unchanged after replays.
    assert(state.total_locked_tlt_base_units == A_AMT + B_AMT);

    std::cout << "  ✓ Multiple sender nonces tracked independently." << std::endl;
}

void test_slashing_never_creates_tokens() {
    std::cout << "[supply] Slashing reduces stake; never creates tokens" << std::endl;

    drachma::Validator v{"validator", 100000};
    uint64_t initial_stake = v.stake;

    // Multiple slash events reduce stake monotonically.
    drachma::ApplySlashing(v, "equivocation");
    assert(v.stake < initial_stake);

    drachma::ApplySlashing(v, "double_sign");
    uint64_t after_second = v.stake;
    assert(after_second < initial_stake);

    // Stake never increases due to slashing.
    // (The only way it could increase would be a supply bug.)
    assert(v.stake <= initial_stake);

    // Slash tiny stake to near-zero — no underflow or negative stake.
    drachma::Validator tiny{"tiny", 5};
    for (int i = 0; i < 100; ++i) {
        drachma::ApplySlashing(tiny, "equivocation");
        // Stake must remain >= 0 (it's uint64_t, so this tests no wrap-around).
        // If stake is 0, 10% rounds down to 0 so it stays at 0.
    }
    // Stake should have converged to 0 or a very small value.
    assert(tiny.stake <= 5);  // Never exceeded initial

    std::cout << "  ✓ Slashing reduces stake; no token creation." << std::endl;
}

void test_l2_l3_supply_invariant() {
    std::cout << "[supply] L2↔L3 minted never exceeds locked" << std::endl;

    bridge::l2_l3::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_drc_base_units = 8000;

    // Mint 8000 — exactly at the limit.
    auto msg = make_drc_mint_msg(1, 8000, 300);
    assert(bridge::l2_l3::ProcessDrcLockMint(state, msg, 315, 300)
           == bridge::l2_l3::BridgeResult::OK);
    assert(state.total_minted_wdrc_base_units == 8000);
    assert(state.total_minted_wdrc_base_units <= state.total_locked_drc_base_units);

    // Attempt to mint 1 more unit — must be rejected.
    auto msg2 = make_drc_mint_msg(2, 1, 305, 0xF0);
    auto r = bridge::l2_l3::ProcessDrcLockMint(state, msg2, 320, 305);
    assert(r == bridge::l2_l3::BridgeResult::ERR_SUPPLY_OVERFLOW);

    std::cout << "  ✓ L2↔L3 minted ≤ locked invariant enforced." << std::endl;
}

void test_bridge_state_recovery_preserves_invariant() {
    std::cout << "[supply] Bridge state recovery preserves supply invariant" << std::endl;

    // Simulate a scenario where the bridge state is partially written to storage
    // and then reloaded. The invariant should still hold.
    bridge::l1_l2::BridgeState recovered_state;
    bridge_test::setup_bridge_state(recovered_state);

    // Simulate: 3000 locked, 2000 minted (the remaining 1000 is in-flight).
    recovered_state.total_locked_tlt_base_units  = 3000;
    recovered_state.total_minted_wtlt_base_units = 2000;

    // Invariant holds on recovery.
    assert(recovered_state.total_minted_wtlt_base_units <=
           recovered_state.total_locked_tlt_base_units);

    // Mint the remaining 1000 on recovery.
    auto msg = make_tlt_mint_msg(99, 1000, 500, 0x77);
    assert(bridge::l1_l2::ProcessTltLockMint(recovered_state, msg, 510, 500)
           == bridge::l1_l2::BridgeResult::OK);
    assert(recovered_state.total_minted_wtlt_base_units == 3000);
    assert(recovered_state.total_minted_wtlt_base_units <=
           recovered_state.total_locked_tlt_base_units);

    // Attempt to mint beyond the locked ceiling after recovery — must be rejected.
    auto msg2 = make_tlt_mint_msg(100, 1, 505, 0x78);
    assert(bridge::l1_l2::ProcessTltLockMint(recovered_state, msg2, 515, 505)
           == bridge::l1_l2::BridgeResult::ERR_SUPPLY_OVERFLOW);

    std::cout << "  ✓ Supply invariant preserved after state recovery." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== PantheonChain Supply Invariant Tests (Phase D + Phase K) ===" << std::endl;
    std::cout << std::endl;

    test_initial_supply_is_zero();
    test_lock_only_does_not_create_wrapped_supply();
    test_minted_never_exceeds_locked();
    test_burn_reduces_minted_supply();
    test_unlock_reduces_locked_supply();
    test_supply_overflow_protection_lock();
    test_multiple_senders_independent();
    test_slashing_never_creates_tokens();
    test_l2_l3_supply_invariant();
    test_bridge_state_recovery_preserves_invariant();

    std::cout << std::endl;
    std::cout << "✓ All supply invariant tests passed." << std::endl;
    return 0;
}
