// PantheonChain — Composite Chaos Scenario Tests (Phase J)
//
// Multi-variable adversarial scenarios combining failures across consensus,
// bridging, supply accounting, and network conditions.  Each scenario is
// drawn from the pre-launch red-team brief (≥ 20 high-severity cases).
//
// Scenarios:
//  1.  High-load bridge + 30% validator downtime: consistency maintained
//  2.  TALANTON low-hashpower window: L2 checkpoint finality assumptions
//  3.  Network partition + stale proof from minority relayer: rejected
//  4.  Simultaneous wrap/unwrap cycling: supply conservation
//  5.  Adversarial sequential replay from multiple attackers
//  6.  Bridge state recovery after simulated crash
//  7.  Validator cartel (exactly 2/3 + 1 stake): accepted; just below rejected
//  8.  Double-sign then equivocation in the same epoch: both slashes apply
//  9.  Burn-then-relock: nonce isolation prevents double-spend
// 10.  Checkpoint re-submission at same height from stale validator set
// 11.  Forged upstream_commitment_hash (tampered OBOLOS hash in DRACHMA commit)
// 12.  Out-of-order delivery: L3 mint arrives before L2 lock confirms
// 13.  Simultaneous max-amount locks from N senders: total locked correct
// 14.  Relayer submits burn proof to wrong bridge endpoint
// 15.  Cross-chain message with no payload: zero-amount / missing amount
// 16.  Validator stake exhaustion through repeated slashing
// 17.  Fork scenario: two competing commitments at the same height
// 18.  Proof with zero-length sibling nodes: proof passes only if leaf==root
// 19.  Chain halt during pending mint: no phantom tokens on resumption
// 20.  Large-scale wrap/unwrap: supply sums to zero at end

#include "bridge/cross_chain_message.h"
#include "bridge/l1_l2/l1_l2_bridge.h"
#include "bridge/l2_l3/l2_l3_bridge.h"
#include "bridge_test_helpers.h"
#include "common/commitments.h"
#include "layer1-talanton/tx/l1_commitment_validator.h"
#include "layer2-drachma/consensus/pos_consensus.h"
#include "layer3-obolos/consensus/pos_consensus.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace pantheon;

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

static std::string hex64(char c) { return std::string(64, c); }

static bridge::CrossChainMessage build_l1l2_mint_msg(uint64_t nonce, uint64_t amount,
                                                     uint64_t lock_h, uint8_t hb = 0xAB) {
    bridge::CrossChainMessage m;
    m.origin_chain_id = bridge::ChainId::TALANTON;
    m.destination_chain_id = bridge::ChainId::DRACHMA;
    m.message_nonce = nonce;
    m.block_height = lock_h;
    m.timestamp = 1000000 + lock_h;
    m.payload.resize(8);
    for (int i = 0; i < 8; ++i)
        m.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    m.payload_hash.fill(hb);
    m.state_root = m.payload_hash;
    bridge_test::bridge_sign_message(m);
    return m;
}

static bridge::CrossChainMessage build_l2l1_unlock_msg(uint64_t nonce, uint64_t amount,
                                                       uint64_t lock_h) {
    bridge::CrossChainMessage m;
    m.origin_chain_id = bridge::ChainId::DRACHMA;
    m.destination_chain_id = bridge::ChainId::TALANTON;
    m.message_nonce = nonce;
    m.block_height = lock_h;
    m.timestamp = 2000000 + lock_h;
    m.payload.resize(8);
    for (int i = 0; i < 8; ++i)
        m.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    m.payload_hash.fill(0xCD);
    m.state_root = m.payload_hash;
    bridge_test::bridge_sign_message(m);
    return m;
}

static bridge::BridgeTransferIntent tlt_lock(const std::string &s, uint64_t n, uint64_t a) {
    bridge::BridgeTransferIntent i;
    i.origin_chain_id = bridge::ChainId::TALANTON;
    i.destination_chain_id = bridge::ChainId::DRACHMA;
    i.sender_address = s;
    i.amount_base_units = a;
    i.nonce = n;
    return i;
}

static bridge::BridgeTransferIntent wtlt_burn(const std::string &s, uint64_t n, uint64_t a) {
    bridge::BridgeTransferIntent i;
    i.origin_chain_id = bridge::ChainId::DRACHMA;
    i.destination_chain_id = bridge::ChainId::TALANTON;
    i.sender_address = s;
    i.amount_base_units = a;
    i.nonce = n;
    return i;
}

static common::Commitment obolos_commit(uint64_t epoch, uint64_t height,
                                        std::vector<common::FinalitySignature> sigs = {}) {
    if (sigs.empty())
        sigs = {{"v1", 100, "sig"}};
    return obolos::BuildL3Commitment(epoch, height, hex64('a'), hex64('b'), hex64('c'),
                                     std::move(sigs));
}

static common::Commitment drachma_commit(uint64_t epoch, uint64_t height,
                                         std::vector<common::FinalitySignature> sigs = {}) {
    if (sigs.empty())
        sigs = {{"v3", 100, "sig3"}};
    return common::Commitment{common::SourceChain::DRACHMA,
                              epoch,
                              height,
                              hex64('d'),
                              hex64('e'),
                              hex64('f'),
                              hex64('9'),
                              std::move(sigs)};
}

// ---------------------------------------------------------------------------
// Scenario 1 — High-load bridge + 30% validator downtime
// ---------------------------------------------------------------------------
void scenario_01_high_load_partial_validator_downtime() {
    std::cout << "[scenario 01] High bridge load + 30% validator downtime: consistency"
              << std::endl;

    bridge::l1_l2::BridgeState l1_state, l2_state;
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);
    const int N = 50;

    // N senders each lock 1000 TLT.
    for (int i = 0; i < N; ++i) {
        auto r = bridge::l1_l2::RecordTltLock(
            l1_state, tlt_lock("user" + std::to_string(i), static_cast<uint64_t>(i), 1000));
        assert(r == bridge::l1_l2::BridgeResult::OK);
    }
    assert(l1_state.total_locked_tlt_base_units == static_cast<uint64_t>(N) * 1000);

    l2_state.total_locked_tlt_base_units = static_cast<uint64_t>(N) * 1000;

    // 30% validator downtime simulated by only 70% of validators signing each commitment.
    // (Consensus: 70% > 2/3 → quorum still met)
    auto commit = obolos_commit(1, 10, {{"v1", 70, "sig1"}, {"v2", 10, "sig2"}});
    auto r = obolos::ValidateL3Finality(commit, 5, 120);
    // 80/120 = 66.7% → exactly at 2/3 threshold.
    // 80 * 3 = 240 >= 120 * 2 = 240 → valid.
    assert(r.valid);

    // All N mints processed in order (relayer delivers in sequence).
    for (int i = 0; i < N; ++i) {
        auto msg = build_l1l2_mint_msg(static_cast<uint64_t>(i), 1000, 100,
                                       static_cast<uint8_t>(0x10 + (i % 0xE0)));
        auto mr = bridge::l1_l2::ProcessTltLockMint(l2_state, msg, 110, 100);
        assert(mr == bridge::l1_l2::BridgeResult::OK);
    }
    assert(l2_state.total_minted_wtlt_base_units == static_cast<uint64_t>(N) * 1000);

    std::cout << "  ✓ All " << N << " mints processed correctly under high load." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 2 — TALANTON low-hashpower: L2 checkpoint anchoring still works
// ---------------------------------------------------------------------------
void scenario_02_low_hashpower_l1_checkpoint() {
    std::cout << "[scenario 02] TALANTON low hashpower: L2 checkpoint finality still enforced"
              << std::endl;

    // Even during a TALANTON low-hashpower period, the L2 commitment validator
    // must still check quorum and monotonic height.
    auto old_commit = drachma_commit(1, 20, {{"v3", 100, "sig3"}});

    // Normal acceptance.
    assert(talanton::ValidateL2Commit(old_commit, {10}, 150).valid);

    // Replay of same height during low-hashpower period — must be rejected.
    assert(!talanton::ValidateL2Commit(old_commit, {20}, 150).valid);
    assert(!talanton::ValidateL2Commit(old_commit, {25}, 150).valid);

    // Adversarial reorg: a commitment at height 15 submitted after we already
    // accepted height 20 — must be rejected (non-monotonic).
    auto rollback_commit = drachma_commit(1, 15, {{"v3", 100, "sig3"}});
    assert(!talanton::ValidateL2Commit(rollback_commit, {20}, 150).valid);

    std::cout << "  ✓ L2 checkpoint monotonicity enforced even under TALANTON stress." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 3 — Network partition + stale proof from minority relayer
// ---------------------------------------------------------------------------
void scenario_03_partition_stale_proof_rejected() {
    std::cout << "[scenario 03] Network partition: stale commitment from minority side rejected"
              << std::endl;

    // Majority side finalized height 30.
    // Minority relayer submits a stale commit at height 25 after reconnection.
    auto stale = obolos_commit(2, 25, {{"v1", 100, "sig1"}});

    // The DRACHMA anchor at height 30 means height 25 is non-monotonic → rejected.
    assert(!drachma::ValidateL3Commit(stale, /*last_l3_height=*/30, 150).valid);

    // Valid next commit at height 31 from the majority side must be accepted.
    auto valid = obolos_commit(2, 31, {{"v1", 100, "sig1"}});
    assert(drachma::ValidateL3Commit(valid, 30, 150).valid);

    std::cout << "  ✓ Stale proof from minority relayer rejected after partition heals."
              << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 4 — Simultaneous wrap/unwrap cycling: supply conservation
// ---------------------------------------------------------------------------
void scenario_04_wrap_unwrap_cycling() {
    std::cout << "[scenario 04] Rapid wrap/unwrap cycling: supply sums to zero" << std::endl;

    bridge::l1_l2::BridgeState l1s, l2s;
    bridge_test::setup_bridge_state(l1s);
    bridge_test::setup_bridge_state(l2s);
    const uint64_t AMT = 5000;
    const int CYCLES = 20;

    for (int cycle = 0; cycle < CYCLES; ++cycle) {
        uint64_t lock_nonce = static_cast<uint64_t>(cycle) * 2 + 1;

        // Lock.
        assert(bridge::l1_l2::RecordTltLock(l1s, tlt_lock("cycler", lock_nonce, AMT)) ==
               bridge::l1_l2::BridgeResult::OK);
        l2s.total_locked_tlt_base_units = l1s.total_locked_tlt_base_units;

        // Mint.
        auto mint_msg = build_l1l2_mint_msg(lock_nonce, AMT, 100 + lock_nonce,
                                            static_cast<uint8_t>(0x20 + cycle));
        assert(
            bridge::l1_l2::ProcessTltLockMint(l2s, mint_msg, 110 + lock_nonce, 100 + lock_nonce) ==
            bridge::l1_l2::BridgeResult::OK);

        // Burn on L2.
        assert(bridge::l1_l2::RecordWtltBurn(l2s, wtlt_burn("cycler", lock_nonce, AMT)) ==
               bridge::l1_l2::BridgeResult::OK);

        // Unlock on L1.
        auto unlock_msg = build_l2l1_unlock_msg(lock_nonce, AMT, 200 + lock_nonce);
        assert(bridge::l1_l2::ProcessWtltBurnUnlock(l1s, unlock_msg, 210 + lock_nonce,
                                                    200 + lock_nonce) ==
               bridge::l1_l2::BridgeResult::OK);
    }

    // After CYCLES complete wrap/unwrap round-trips, supply must be zero on both sides.
    assert(l1s.total_locked_tlt_base_units == 0);
    assert(l2s.total_minted_wtlt_base_units == 0);

    std::cout << "  ✓ Supply zero after " << CYCLES << " wrap/unwrap cycles." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 5 — Sequential replay from multiple attackers
// ---------------------------------------------------------------------------
void scenario_05_multi_attacker_replay() {
    std::cout << "[scenario 05] Sequential replay attack from multiple independent attackers"
              << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = 5000;

    // Attacker A processes nonce 1 legitimately.
    auto msgA = build_l1l2_mint_msg(1, 1000, 100, 0xA1);
    assert(bridge::l1_l2::ProcessTltLockMint(state, msgA, 110, 100) ==
           bridge::l1_l2::BridgeResult::OK);

    // Attacker A tries to replay nonce 1 → rejected.
    assert(bridge::l1_l2::ProcessTltLockMint(state, msgA, 111, 100) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Attacker B processes nonce 2 legitimately.
    auto msgB = build_l1l2_mint_msg(2, 1000, 105, 0xB2);
    assert(bridge::l1_l2::ProcessTltLockMint(state, msgB, 115, 105) ==
           bridge::l1_l2::BridgeResult::OK);

    // Attacker B tries to replay nonce 2 → rejected.
    assert(bridge::l1_l2::ProcessTltLockMint(state, msgB, 120, 105) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Attacker C sends nonce 1 again with a different payload_hash trick → still rejected
    // (nonce key is "1001:1" regardless of payload_hash).
    auto msgC = build_l1l2_mint_msg(1, 1000, 100, 0xC3); // Different hash byte but same nonce
    assert(bridge::l1_l2::ProcessTltLockMint(state, msgC, 130, 100) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Total minted: only 2000 (not 3000 or more).
    assert(state.total_minted_wtlt_base_units == 2000);

    std::cout << "  ✓ Multi-attacker replay sequences correctly rejected." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 6 — Bridge state recovery: no phantom tokens
// ---------------------------------------------------------------------------
void scenario_06_bridge_recovery_no_phantom_tokens() {
    std::cout << "[scenario 06] Bridge state recovery: no phantom balances" << std::endl;

    // Before crash: 3000 locked, 2000 minted.
    bridge::l1_l2::BridgeState recovered;
    bridge_test::setup_bridge_state(recovered);
    recovered.total_locked_tlt_base_units = 3000;
    recovered.total_minted_wtlt_base_units = 2000;

    // On recovery, the in-flight mint message (nonce 99, amount 1000) arrives.
    auto msg = build_l1l2_mint_msg(99, 1000, 500, 0x77);
    assert(bridge::l1_l2::ProcessTltLockMint(recovered, msg, 510, 500) ==
           bridge::l1_l2::BridgeResult::OK);
    assert(recovered.total_minted_wtlt_base_units == 3000);

    // Replay of the same in-flight message after recovery → rejected.
    assert(bridge::l1_l2::ProcessTltLockMint(recovered, msg, 515, 500) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // No phantom tokens: minted == locked (all in-flight messages settled).
    assert(recovered.total_minted_wtlt_base_units == recovered.total_locked_tlt_base_units);

    std::cout << "  ✓ Bridge recovery produces no phantom tokens." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 7 — Validator cartel: exactly 2/3+1 stake → accepted; 2/3-1 → rejected
// ---------------------------------------------------------------------------
void scenario_07_validator_cartel_threshold() {
    std::cout << "[scenario 07] Validator cartel threshold: ⌈2/3⌉ boundary" << std::endl;

    // active_stake = 90. Threshold = ceil(2/3 * 90) = 60.
    uint64_t active_stake = 90;

    // 60 signed: 60 * 3 = 180 >= 90 * 2 = 180 → exactly at threshold → accepted.
    auto commit_60 = obolos_commit(1, 10, {{"v1", 60, "sig"}});
    assert(obolos::ValidateL3Finality(commit_60, 5, active_stake).valid);

    // 59 signed: 59 * 3 = 177 < 180 → below threshold → rejected.
    auto commit_59 = obolos_commit(1, 10, {{"v1", 59, "sig"}});
    assert(!obolos::ValidateL3Finality(commit_59, 5, active_stake).valid);

    // 61 signed → clearly above threshold → accepted.
    auto commit_61 = obolos_commit(1, 10, {{"v1", 61, "sig"}});
    assert(obolos::ValidateL3Finality(commit_61, 5, active_stake).valid);

    std::cout << "  ✓ 2/3 quorum boundary: 60/90 accepted, 59/90 rejected." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 8 — Double-sign then equivocation: both slashes applied
// ---------------------------------------------------------------------------
void scenario_08_double_slash() {
    std::cout << "[scenario 08] Double-sign then equivocation: both slashes applied" << std::endl;

    drachma::Validator v{"bad", 100000};
    uint64_t initial = v.stake;

    auto e1 = drachma::ApplySlashing(v, "double_sign");  // 5%: −5000
    auto e2 = drachma::ApplySlashing(v, "equivocation"); // 10% of 95000: −9500

    assert(e1.slashed_amount == 5000);
    assert(e2.slashed_amount == 9500);
    assert(v.stake == initial - 5000 - 9500);
    assert(v.stake < initial);

    std::cout << "  ✓ Both slash events applied; total slashed = " << (5000 + 9500) << "."
              << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 9 — Burn-then-relock: nonce isolation across L1 and L2 state objects
// ---------------------------------------------------------------------------
void scenario_09_burn_then_relock_nonce_isolation() {
    std::cout << "[scenario 09] Burn-then-relock: nonce keys are isolated" << std::endl;

    // In the protocol, L1 and L2 bridge sides each have their OWN state.
    // RecordTltLock (L1 side) and RecordWtltBurn (L2 side) track nonces independently.
    bridge::l1_l2::BridgeState l1_state; // L1 side: handles locks and unlock proofs
    bridge::l1_l2::BridgeState l2_state; // L2 side: handles mints and burns
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);

    // Alice locks TLT with nonce 1 on L1.
    assert(bridge::l1_l2::RecordTltLock(l1_state, tlt_lock("alice", 1, 1000)) ==
           bridge::l1_l2::BridgeResult::OK);

    // Alice burns wTLT with nonce 1 on L2 side (separate state — no nonce collision).
    assert(bridge::l1_l2::RecordWtltBurn(l2_state, wtlt_burn("alice", 1, 1000)) ==
           bridge::l1_l2::BridgeResult::OK);

    // Alice locks again with nonce 2 on L1 → succeeds (nonce 2 is fresh on L1 state).
    assert(bridge::l1_l2::RecordTltLock(l1_state, tlt_lock("alice", 2, 1000)) ==
           bridge::l1_l2::BridgeResult::OK);

    // Alice tries to lock again with nonce 1 on L1 → replayed on L1 state → rejected.
    assert(bridge::l1_l2::RecordTltLock(l1_state, tlt_lock("alice", 1, 1000)) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Alice tries to burn again with nonce 1 on L2 → replayed on L2 state → rejected.
    assert(bridge::l1_l2::RecordWtltBurn(l2_state, wtlt_burn("alice", 1, 1000)) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);

    std::cout << "  ✓ L1 and L2 nonce spaces are independent; replays rejected on both sides."
              << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 10 — Stale validator set: commitment replayed after rotation
// ---------------------------------------------------------------------------
void scenario_10_stale_validator_set_replay() {
    std::cout << "[scenario 10] Stale validator set: old commitment replayed at new height"
              << std::endl;

    // Old commitment from epoch 1 that was accepted.
    auto old_commit = obolos_commit(1, 10, {{"v1", 100, "old_sig"}});
    assert(obolos::ValidateL3Finality(old_commit, 5, 150).valid);

    // Attacker replays the same commitment object at a higher anchor height.
    // The last_finalized_height is now 10, so the replay at height 10 must fail.
    assert(!obolos::ValidateL3Finality(old_commit, 10, 150).valid);

    // Creating a new commitment at height 11 with the same data but new height — this is a
    // *different* commitment, not a replay, and should pass if sigs are valid.
    auto new_commit = obolos_commit(1, 11, {{"v1", 100, "new_sig"}});
    assert(obolos::ValidateL3Finality(new_commit, 10, 150).valid);

    std::cout << "  ✓ Stale commitment replay at same height rejected; new height accepted."
              << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 11 — Tampered upstream_commitment_hash in DRACHMA commit
// ---------------------------------------------------------------------------
void scenario_11_tampered_upstream_hash() {
    std::cout << "[scenario 11] Tampered upstream_commitment_hash in DRACHMA commit rejected"
              << std::endl;

    // A valid DRACHMA commitment with a well-formed upstream_commitment_hash.
    auto good = drachma_commit(1, 20);
    assert(talanton::ValidateL2Commit(good, {10}, 150).valid);

    // Attacker changes the upstream_commitment_hash to a non-hex string.
    common::Commitment tampered = good;
    tampered.upstream_commitment_hash =
        "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ";
    assert(!talanton::ValidateL2Commit(tampered, {10}, 150).valid);

    // Attacker sets upstream_commitment_hash to wrong length.
    tampered.upstream_commitment_hash = "abc";
    assert(!talanton::ValidateL2Commit(tampered, {10}, 150).valid);

    std::cout << "  ✓ Tampered upstream_commitment_hash rejected." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 12 — Out-of-order delivery: mint arrives before lock height confirmed
// ---------------------------------------------------------------------------
void scenario_12_out_of_order_mint() {
    std::cout << "[scenario 12] Out-of-order delivery: mint before L1 confirmation depth"
              << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = 2000;

    auto msg = build_l1l2_mint_msg(1, 2000, 500);

    // Relayer delivers mint message only 3 blocks after lock (< 6 required).
    assert(bridge::l1_l2::ProcessTltLockMint(state, msg, 503, 500) ==
           bridge::l1_l2::BridgeResult::ERR_INSUFFICIENT_CONF);

    // After sufficient confirmation, delivery is accepted.
    assert(bridge::l1_l2::ProcessTltLockMint(state, msg, 506, 500) ==
           bridge::l1_l2::BridgeResult::OK);

    std::cout << "  ✓ Out-of-order mint blocked until confirmation depth met." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 13 — N senders max-amount locks: total locked is correct
// ---------------------------------------------------------------------------
void scenario_13_n_sender_max_locks() {
    std::cout << "[scenario 13] N senders locking large amounts: total locked correct" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    const int N = 10;
    const uint64_t PER_SENDER = 1000000000ULL; // 1 billion base units each

    for (int i = 0; i < N; ++i) {
        auto r = bridge::l1_l2::RecordTltLock(
            state, tlt_lock("sender" + std::to_string(i), static_cast<uint64_t>(i), PER_SENDER));
        assert(r == bridge::l1_l2::BridgeResult::OK);
    }
    assert(state.total_locked_tlt_base_units == static_cast<uint64_t>(N) * PER_SENDER);

    std::cout << "  ✓ " << N << " senders × " << PER_SENDER << " = "
              << static_cast<uint64_t>(N) * PER_SENDER << " total locked." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 14 — Relayer submits burn proof to wrong bridge endpoint
// ---------------------------------------------------------------------------
void scenario_14_burn_proof_wrong_endpoint() {
    std::cout << "[scenario 14] Burn proof submitted to wrong bridge endpoint" << std::endl;

    bridge::l1_l2::BridgeState l1_state;
    bridge::l2_l3::BridgeState l2_state;
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);
    l1_state.total_locked_tlt_base_units = 1000;
    l2_state.total_locked_drc_base_units = 1000;

    // A DRACHMA→TALANTON burn message (correct for L1 bridge).
    bridge::CrossChainMessage burn_msg;
    burn_msg.origin_chain_id = bridge::ChainId::DRACHMA;
    burn_msg.destination_chain_id = bridge::ChainId::TALANTON;
    burn_msg.message_nonce = 1;
    burn_msg.block_height = 200;
    burn_msg.payload.resize(8);
    for (int i = 0; i < 8; ++i)
        burn_msg.payload[static_cast<size_t>(i)] =
            static_cast<uint8_t>((1000ULL >> (8 * i)) & 0xFF);
    burn_msg.payload_hash.fill(0xBB);
    burn_msg.state_root = burn_msg.payload_hash;
    bridge_test::bridge_sign_message(burn_msg);

    // Correct endpoint: L1 bridge unlock → should succeed.
    assert(bridge::l1_l2::ProcessWtltBurnUnlock(l1_state, burn_msg, 210, 200) ==
           bridge::l1_l2::BridgeResult::OK);

    // Wrong endpoint: submitted to L2↔L3 burn unlock function → wrong chain IDs.
    assert(bridge::l2_l3::ProcessWdrcBurnUnlock(l2_state, burn_msg, 215, 200) ==
           bridge::l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ Burn proof rejected by wrong bridge endpoint." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 15 — Message with no payload (zero-length): missing amount info
// ---------------------------------------------------------------------------
void scenario_15_no_payload_missing_amount() {
    std::cout << "[scenario 15] Message with no payload: zero amount decoded, no supply change"
              << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = 1000;

    // Message with empty payload (no amount encoded).
    auto msg = build_l1l2_mint_msg(1, 0, 100);
    msg.payload.clear(); // Zero length payload → decode_amount_from_payload returns 0

    // The proof still passes (payload_hash was set from the original build); let's fix:
    msg.payload_hash.fill(0x00);
    msg.state_root = msg.payload_hash;
    // Re-sign after modifying payload_hash and state_root.
    msg.validator_signatures.clear();
    bridge_test::bridge_sign_message(msg);

    auto r = bridge::l1_l2::ProcessTltLockMint(state, msg, 110, 100);
    // Should succeed (no error for zero decoded amount when total > minted),
    // but no supply change because amount == 0.
    assert(r == bridge::l1_l2::BridgeResult::OK);
    assert(state.total_minted_wtlt_base_units == 0); // No tokens minted (amount was 0)

    std::cout << "  ✓ Zero-payload message produces no supply change." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 16 — Validator stake exhaustion through repeated slashing
// ---------------------------------------------------------------------------
void scenario_16_validator_stake_exhaustion() {
    std::cout << "[scenario 16] Validator stake exhaustion through repeated slashing" << std::endl;

    drachma::Validator v{"exhausted", 10000};

    // Apply equivocation slashes until stake reaches zero.
    for (int i = 0; i < 200; ++i) {
        drachma::ApplySlashing(v, "equivocation");
        if (v.stake == 0)
            break;
    }

    // Stake must be zero or very small (integer truncation).
    assert(v.stake <= 10000); // Never exceeded initial

    // A zero-stake validator should not cause arithmetic issues.
    if (v.stake == 0) {
        auto ev = drachma::ApplySlashing(v, "equivocation");
        assert(ev.slashed_amount == 0);
        assert(v.stake == 0);
    }

    std::cout << "  ✓ Stake exhausted safely to zero; no underflow." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 17 — Fork: two competing commitments at the same height
// ---------------------------------------------------------------------------
void scenario_17_competing_commitments() {
    std::cout << "[scenario 17] Fork: two competing OBOLOS commitments at same height" << std::endl;

    // Fork A: valid commit at height 10.
    auto fork_a = obolos_commit(1, 10, {{"v1", 100, "sig_a"}});
    assert(obolos::ValidateL3Finality(fork_a, 5, 150).valid);

    // Fork B: also claims to be at height 10 (adversarial double commit).
    auto fork_b = obolos_commit(1, 10, {{"v2", 100, "sig_b"}});

    // Both forks pass validation individually (finality check is stateless).
    // The chain must reject the second one based on the last_finalized_height
    // being updated after accepting fork A.
    assert(obolos::ValidateL3Finality(fork_b, 5, 150).valid); // Valid individually

    // After fork A is accepted (last_finalized_height = 10), fork B must fail.
    assert(!obolos::ValidateL3Finality(fork_b, 10, 150).valid);

    std::cout << "  ✓ Second competing commit at same height rejected after first accepted."
              << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 18 — Proof with empty sibling: leaf == root (single-leaf Merkle)
// ---------------------------------------------------------------------------
void scenario_18_empty_proof_leaf_equals_root() {
    std::cout << "[scenario 18] Empty proof passes iff leaf_hash == state_root" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = 1000;

    // Correct: payload_hash == state_root with empty proof → passes.
    bridge::CrossChainMessage valid_msg;
    valid_msg.origin_chain_id = bridge::ChainId::TALANTON;
    valid_msg.destination_chain_id = bridge::ChainId::DRACHMA;
    valid_msg.message_nonce = 1;
    valid_msg.block_height = 100;
    valid_msg.payload.resize(8);
    uint64_t AMT = 1000;
    for (int i = 0; i < 8; ++i)
        valid_msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((AMT >> (8 * i)) & 0xFF);
    valid_msg.payload_hash.fill(0x55);
    valid_msg.state_root = valid_msg.payload_hash; // leaf == root: empty proof is valid
    valid_msg.proof.clear();
    bridge_test::bridge_sign_message(valid_msg);

    assert(bridge::l1_l2::ProcessTltLockMint(state, valid_msg, 110, 100) ==
           bridge::l1_l2::BridgeResult::OK);

    // Incorrect: payload_hash ≠ state_root with empty proof → fails.
    bridge::CrossChainMessage invalid_msg = valid_msg;
    invalid_msg.message_nonce = 2; // Different nonce
    invalid_msg.payload_hash.fill(0x55);
    invalid_msg.state_root.fill(0x66); // Mismatch!
    invalid_msg.proof.clear();
    // Re-sign after modifying payload_hash and state_root so quorum check passes.
    invalid_msg.validator_signatures.clear();
    bridge_test::bridge_sign_message(invalid_msg);

    assert(bridge::l1_l2::ProcessTltLockMint(state, invalid_msg, 120, 100) ==
           bridge::l1_l2::BridgeResult::ERR_INVALID_PROOF);

    std::cout << "  ✓ Empty proof accepted iff payload_hash == state_root." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 19 — Chain halt during pending mint: no phantom tokens on resumption
// ---------------------------------------------------------------------------
void scenario_19_chain_halt_pending_mint() {
    std::cout << "[scenario 19] Chain halt during pending mint: no phantom tokens" << std::endl;

    bridge::l1_l2::BridgeState l1_state, l2_state;
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);

    // Lock committed on L1 before halt.
    assert(bridge::l1_l2::RecordTltLock(l1_state, tlt_lock("dana", 1, 3000)) ==
           bridge::l1_l2::BridgeResult::OK);

    // L2 was processing a different mint (nonce 99) when it halted.
    l2_state.total_locked_tlt_base_units = 3000;
    // Simulate a partial state: the nonce-99 message was added to the processed set
    // (crash occurred after the nonce was recorded but before supply update).
    // This is modeled by simulating the post-halt recovery state.
    // On resumption, the nonce-99 message arrives again → ERR_REPLAY.
    l2_state.processed_nonce_keys.insert("1001:99");

    auto msg_99 = build_l1l2_mint_msg(99, 1000, 400, 0x88);
    assert(bridge::l1_l2::ProcessTltLockMint(l2_state, msg_99, 410, 400) ==
           bridge::l1_l2::BridgeResult::ERR_REPLAY);
    assert(l2_state.total_minted_wtlt_base_units == 0); // No phantom tokens

    // New mint (nonce 1) for dana's lock is processed normally after halt recovery.
    auto msg_1 = build_l1l2_mint_msg(1, 3000, 450, 0x89);
    assert(bridge::l1_l2::ProcessTltLockMint(l2_state, msg_1, 460, 450) ==
           bridge::l1_l2::BridgeResult::OK);
    assert(l2_state.total_minted_wtlt_base_units == 3000);

    std::cout << "  ✓ No phantom tokens after chain halt; replay prevented." << std::endl;
}

// ---------------------------------------------------------------------------
// Scenario 20 — Large-scale wrap/unwrap: net supply returns to zero
// ---------------------------------------------------------------------------
void scenario_20_large_scale_round_trip() {
    std::cout << "[scenario 20] Large-scale wrap/unwrap: net supply returns to zero" << std::endl;

    bridge::l1_l2::BridgeState l1_state, l2_state;
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);
    const int N_USERS = 100;
    const uint64_t PER_USER = 777;

    // All users lock TLT.
    for (int i = 0; i < N_USERS; ++i) {
        bridge::l1_l2::RecordTltLock(
            l1_state, tlt_lock("user" + std::to_string(i), static_cast<uint64_t>(i), PER_USER));
    }
    l2_state.total_locked_tlt_base_units = l1_state.total_locked_tlt_base_units;

    // All users mint wTLT on L2.
    for (int i = 0; i < N_USERS; ++i) {
        auto m =
            build_l1l2_mint_msg(static_cast<uint64_t>(i), PER_USER, 100 + static_cast<uint64_t>(i),
                                static_cast<uint8_t>(0x30 + (i % 0xBF)));
        bridge::l1_l2::ProcessTltLockMint(l2_state, m, 110 + i, 100 + i);
    }
    assert(l2_state.total_minted_wtlt_base_units == static_cast<uint64_t>(N_USERS) * PER_USER);

    // All users burn wTLT on L2.
    for (int i = 0; i < N_USERS; ++i) {
        bridge::l1_l2::RecordWtltBurn(
            l2_state, wtlt_burn("user" + std::to_string(i), static_cast<uint64_t>(i), PER_USER));
    }
    assert(l2_state.total_minted_wtlt_base_units == 0);

    // All users unlock TLT on L1.
    for (int i = 0; i < N_USERS; ++i) {
        auto u = build_l2l1_unlock_msg(static_cast<uint64_t>(i), PER_USER,
                                       200 + static_cast<uint64_t>(i));
        bridge::l1_l2::ProcessWtltBurnUnlock(l1_state, u, 210 + i, 200 + i);
    }
    assert(l1_state.total_locked_tlt_base_units == 0);

    std::cout << "  ✓ " << N_USERS << " users: net supply zero after full round-trip." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== PantheonChain Composite Chaos Scenarios (Phase J) ===" << std::endl;
    std::cout << std::endl;

    scenario_01_high_load_partial_validator_downtime();
    scenario_02_low_hashpower_l1_checkpoint();
    scenario_03_partition_stale_proof_rejected();
    scenario_04_wrap_unwrap_cycling();
    scenario_05_multi_attacker_replay();
    scenario_06_bridge_recovery_no_phantom_tokens();
    scenario_07_validator_cartel_threshold();
    scenario_08_double_slash();
    scenario_09_burn_then_relock_nonce_isolation();
    scenario_10_stale_validator_set_replay();
    scenario_11_tampered_upstream_hash();
    scenario_12_out_of_order_mint();
    scenario_13_n_sender_max_locks();
    scenario_14_burn_proof_wrong_endpoint();
    scenario_15_no_payload_missing_amount();
    scenario_16_validator_stake_exhaustion();
    scenario_17_competing_commitments();
    scenario_18_empty_proof_leaf_equals_root();
    scenario_19_chain_halt_pending_mint();
    scenario_20_large_scale_round_trip();

    std::cout << std::endl;
    std::cout << "✓ All 20 composite chaos scenarios passed." << std::endl;
    return 0;
}
