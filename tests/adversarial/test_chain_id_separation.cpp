// PantheonChain — Chain ID / Wallet Signing Separation Tests (Phase G)
//
// Verifies that chain identities are rigorously separated:
//   • each chain has a unique, canonical chain ID
//   • cross-chain messages are rejected by any bridge that does not own the pair
//   • OBOLOS commitments cannot be submitted as DRACHMA commitments (and vice versa)
//   • DRACHMA commitments without upstream_commitment_hash are rejected
//   • wrong-direction bridge messages are rejected
//   • L1→L3 direct hop (bypassing L2) is rejected by both bridges
//   • chain ID enum values match the PantheonChain specification (1001/1002/1003)
//   • payments to wrong chain asset are rejected

#include "bridge/cross_chain_message.h"
#include "bridge/l1_l2/l1_l2_bridge.h"
#include "bridge/l2_l3/l2_l3_bridge.h"
#include "common/commitments.h"
#include "layer1-talanton/tx/l1_commitment_validator.h"
#include "layer2-drachma/consensus/pos_consensus.h"
#include "layer3-obolos/consensus/pos_consensus.h"
#include "bridge_test_helpers.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

using namespace pantheon;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string hex64(char c) { return std::string(64, c); }

static common::Commitment make_obolos_commit(uint64_t height = 10)
{
    return obolos::BuildL3Commitment(1, height, hex64('a'), hex64('b'), hex64('c'),
                                     {{"v1", 100, "sig"}});
}

static common::Commitment make_drachma_commit(uint64_t height = 20)
{
    return common::Commitment{
        common::SourceChain::DRACHMA, 1, height,
        hex64('d'), hex64('e'), hex64('f'), hex64('9'),
        {{"v3", 100, "sig3"}}};
}

static bridge::CrossChainMessage make_message(bridge::ChainId origin, bridge::ChainId dest,
                                              uint64_t nonce, uint64_t amount)
{
    bridge::CrossChainMessage msg;
    msg.origin_chain_id      = origin;
    msg.destination_chain_id = dest;
    msg.message_nonce        = nonce;
    msg.block_height         = 100;
    msg.timestamp            = 1000100;
    msg.payload.resize(8);
    for (int i = 0; i < 8; ++i)
        msg.payload[static_cast<size_t>(i)] = static_cast<uint8_t>((amount >> (8 * i)) & 0xFF);
    msg.payload_hash.fill(0xAA);
    msg.state_root = msg.payload_hash;  // empty proof passes
    bridge_test::bridge_sign_message(msg);
    return msg;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_chain_id_values_match_spec() {
    std::cout << "[chain_id] Chain ID values match the PantheonChain specification" << std::endl;

    // Per the PantheonChain whitepaper and cross_chain_message.h:
    //   TALANTON = 1001, DRACHMA = 1002, OBOLOS = 1003
    assert(static_cast<uint32_t>(bridge::ChainId::TALANTON) == 1001u);
    assert(static_cast<uint32_t>(bridge::ChainId::DRACHMA)  == 1002u);
    assert(static_cast<uint32_t>(bridge::ChainId::OBOLOS)   == 1003u);

    // Chain IDs must be distinct.
    assert(bridge::ChainId::TALANTON != bridge::ChainId::DRACHMA);
    assert(bridge::ChainId::DRACHMA  != bridge::ChainId::OBOLOS);
    assert(bridge::ChainId::TALANTON != bridge::ChainId::OBOLOS);

    std::cout << "  ✓ Chain IDs: TALANTON=1001, DRACHMA=1002, OBOLOS=1003." << std::endl;
}

void test_l1_bridge_rejects_non_l1_messages() {
    std::cout << "[chain_id] L1↔L2 bridge rejects messages not addressed to it" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_tlt_base_units = 1000;

    // L2→L3 message sent to L1↔L2 mint function.
    auto wrong1 = make_message(bridge::ChainId::DRACHMA, bridge::ChainId::OBOLOS, 1, 1000);
    assert(bridge::l1_l2::ProcessTltLockMint(state, wrong1, 110, 100)
           == bridge::l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // L3→L2 message sent to L1↔L2 unlock function.
    auto wrong2 = make_message(bridge::ChainId::OBOLOS, bridge::ChainId::DRACHMA, 1, 1000);
    assert(bridge::l1_l2::ProcessWtltBurnUnlock(state, wrong2, 110, 100)
           == bridge::l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // L1→L3 direct hop (bypassing L2) sent to L1↔L2 mint function.
    auto wrong3 = make_message(bridge::ChainId::TALANTON, bridge::ChainId::OBOLOS, 2, 1000);
    assert(bridge::l1_l2::ProcessTltLockMint(state, wrong3, 110, 100)
           == bridge::l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ L1↔L2 bridge rejects all non-L1↔L2 message pairs." << std::endl;
}

void test_l2_bridge_rejects_non_l2_messages() {
    std::cout << "[chain_id] L2↔L3 bridge rejects messages not addressed to it" << std::endl;

    bridge::l2_l3::BridgeState state;
    bridge_test::setup_bridge_state(state);
    state.total_locked_drc_base_units = 1000;

    // L1→L2 message sent to L2↔L3 mint function.
    auto wrong1 = make_message(bridge::ChainId::TALANTON, bridge::ChainId::DRACHMA, 1, 1000);
    assert(bridge::l2_l3::ProcessDrcLockMint(state, wrong1, 115, 100)
           == bridge::l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    // L1→L2 message sent to L2↔L3 unlock function.
    assert(bridge::l2_l3::ProcessWdrcBurnUnlock(state, wrong1, 115, 100)
           == bridge::l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    // L3→L1 direct hop (bypassing L2) sent to L2↔L3 bridge.
    auto wrong2 = make_message(bridge::ChainId::OBOLOS, bridge::ChainId::TALANTON, 2, 1000);
    assert(bridge::l2_l3::ProcessDrcLockMint(state, wrong2, 115, 100)
           == bridge::l2_l3::BridgeResult::ERR_INVALID_CHAIN);
    assert(bridge::l2_l3::ProcessWdrcBurnUnlock(state, wrong2, 115, 100)
           == bridge::l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ L2↔L3 bridge rejects all non-L2↔L3 message pairs." << std::endl;
}

void test_l1_to_l3_direct_hop_rejected() {
    std::cout << "[chain_id] Direct L1→L3 hop rejected by both bridges" << std::endl;

    bridge::l1_l2::BridgeState l1_state;
    bridge::l2_l3::BridgeState l2_state;
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);
    l1_state.total_locked_tlt_base_units = 1000;
    l2_state.total_locked_drc_base_units = 1000;

    auto l1_to_l3 = make_message(bridge::ChainId::TALANTON, bridge::ChainId::OBOLOS, 1, 1000);

    // L1↔L2 bridge: wrong destination.
    assert(bridge::l1_l2::ProcessTltLockMint(l1_state, l1_to_l3, 110, 100)
           == bridge::l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // L2↔L3 bridge: wrong origin.
    assert(bridge::l2_l3::ProcessDrcLockMint(l2_state, l1_to_l3, 115, 100)
           == bridge::l2_l3::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ Direct L1→L3 hop rejected by both bridges." << std::endl;
}

void test_commitment_chain_domain_separation() {
    std::cout << "[chain_id] OBOLOS commitment rejected by TALANTON as if it were DRACHMA" << std::endl;

    // An OBOLOS commitment (source_chain == OBOLOS) submitted to TALANTON's L2 validator.
    // TALANTON expects source_chain == DRACHMA.
    auto obolos_commit = make_obolos_commit(10);
    auto result = talanton::ValidateL2Commit(obolos_commit, {5}, 150);
    assert(!result.valid);

    // A DRACHMA commitment submitted to OBOLOS's L3 finality validator.
    // OBOLOS expects source_chain == OBOLOS.
    auto drachma_commit = make_drachma_commit(20);
    auto r2 = obolos::ValidateL3Finality(drachma_commit, 10, 150);
    assert(!r2.valid);

    std::cout << "  ✓ Cross-layer commitment domain separation enforced." << std::endl;
}

void test_drachma_commit_requires_upstream_hash() {
    std::cout << "[chain_id] DRACHMA commitment requires upstream_commitment_hash" << std::endl;

    // Commitment without upstream hash must be rejected at encoding validation.
    common::Commitment bad{
        common::SourceChain::DRACHMA, 1, 20,
        hex64('d'), hex64('e'), hex64('f'), "",  // empty upstream hash
        {{"v3", 100, "sig3"}}};

    assert(!common::ValidatePayloadEncoding(bad).valid);

    // Malformed upstream hash (wrong length) must also be rejected.
    common::Commitment bad2{
        common::SourceChain::DRACHMA, 1, 20,
        hex64('d'), hex64('e'), hex64('f'),
        "not_64_chars",  // wrong length
        {{"v3", 100, "sig3"}}};
    assert(!common::ValidatePayloadEncoding(bad2).valid);

    std::cout << "  ✓ DRACHMA commit without valid upstream_commitment_hash rejected." << std::endl;
}

void test_obolos_commit_source_chain_name() {
    std::cout << "[chain_id] SourceChain name helpers return correct strings" << std::endl;

    assert(common::SourceChainName(common::SourceChain::DRACHMA) == "DRACHMA");
    assert(common::SourceChainName(common::SourceChain::OBOLOS)  == "OBOLOS");

    std::cout << "  ✓ SourceChain name helpers correct." << std::endl;
}

void test_wrong_direction_bridge_intent() {
    std::cout << "[chain_id] Wrong-direction BridgeTransferIntent rejected" << std::endl;

    bridge::l1_l2::BridgeState state;
    bridge_test::setup_bridge_state(state);

    // Lock intent with destination == OBOLOS (should be DRACHMA for L1↔L2 bridge).
    bridge::BridgeTransferIntent bad_lock;
    bad_lock.origin_chain_id      = bridge::ChainId::TALANTON;
    bad_lock.destination_chain_id = bridge::ChainId::OBOLOS;
    bad_lock.sender_address       = "alice";
    bad_lock.amount_base_units    = 1000;
    bad_lock.nonce                = 1;
    assert(bridge::l1_l2::RecordTltLock(state, bad_lock)
           == bridge::l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    // Burn intent with origin == TALANTON (should be DRACHMA for L1↔L2 bridge).
    bridge::BridgeTransferIntent bad_burn;
    bad_burn.origin_chain_id      = bridge::ChainId::TALANTON;
    bad_burn.destination_chain_id = bridge::ChainId::TALANTON;
    bad_burn.sender_address       = "alice";
    bad_burn.amount_base_units    = 1000;
    bad_burn.nonce                = 1;
    assert(bridge::l1_l2::RecordWtltBurn(state, bad_burn)
           == bridge::l1_l2::BridgeResult::ERR_INVALID_CHAIN);

    std::cout << "  ✓ Wrong-direction BridgeTransferIntents rejected." << std::endl;
}

void test_message_nonce_includes_chain_context() {
    std::cout << "[chain_id] Nonce keys are scoped per (chain_id, nonce) — no cross-bridge collision" << std::endl;

    // L1↔L2 bridge uses key = "1001:<nonce>" for ProcessTltLockMint.
    // L2↔L3 bridge uses key = "1002:<nonce>" for ProcessDrcLockMint.
    // These are different keys, so the same numeric nonce on different bridges is independent.

    bridge::l1_l2::BridgeState l1_state;
    bridge::l2_l3::BridgeState l2_state;
    bridge_test::setup_bridge_state(l1_state);
    bridge_test::setup_bridge_state(l2_state);
    l1_state.total_locked_tlt_base_units = 1000;
    l2_state.total_locked_drc_base_units = 1000;

    auto l1_msg = make_message(bridge::ChainId::TALANTON, bridge::ChainId::DRACHMA, 5, 1000);
    auto l2_msg = make_message(bridge::ChainId::DRACHMA, bridge::ChainId::OBOLOS, 5, 1000);

    // Both bridges accept nonce 5 independently.
    assert(bridge::l1_l2::ProcessTltLockMint(l1_state, l1_msg, 110, 100)
           == bridge::l1_l2::BridgeResult::OK);
    assert(bridge::l2_l3::ProcessDrcLockMint(l2_state, l2_msg, 315, 300)
           == bridge::l2_l3::BridgeResult::OK);

    // Replaying nonce 5 on the L1↔L2 bridge fails.
    assert(bridge::l1_l2::ProcessTltLockMint(l1_state, l1_msg, 120, 110)
           == bridge::l1_l2::BridgeResult::ERR_REPLAY);

    // Replaying nonce 5 on the L2↔L3 bridge also fails.
    assert(bridge::l2_l3::ProcessDrcLockMint(l2_state, l2_msg, 325, 310)
           == bridge::l2_l3::BridgeResult::ERR_REPLAY);

    std::cout << "  ✓ Nonce tracking is per-bridge; no cross-bridge nonce collision." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== PantheonChain Chain ID / Signing Separation Tests (Phase G) ===" << std::endl;
    std::cout << std::endl;

    test_chain_id_values_match_spec();
    test_l1_bridge_rejects_non_l1_messages();
    test_l2_bridge_rejects_non_l2_messages();
    test_l1_to_l3_direct_hop_rejected();
    test_commitment_chain_domain_separation();
    test_drachma_commit_requires_upstream_hash();
    test_obolos_commit_source_chain_name();
    test_wrong_direction_bridge_intent();
    test_message_nonce_includes_chain_context();

    std::cout << std::endl;
    std::cout << "✓ All chain ID / signing separation tests passed." << std::endl;
    return 0;
}
