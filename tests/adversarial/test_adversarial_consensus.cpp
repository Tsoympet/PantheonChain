// PantheonChain — Adversarial Consensus Tests (Phase B)
//
// Tests every category of consensus attack described in the pre-launch adversarial
// simulation campaign:
//   • height-replay / non-monotonic height rejection
//   • forged source-chain rejection
//   • insufficient quorum (< 2/3) rejection
//   • duplicate signer deduplication
//   • zero active stake rejection
//   • empty signature set rejection
//   • boundary quorum (exactly ⌈2/3⌉ accepted)
//   • L3 commitment anchoring to L2 and L2 commitment anchoring to L1
//   • slashing: double_sign and equivocation
//   • slashing: unknown reason throws
//   • slashing: never reduces stake below zero
//   • proposer determinism: same inputs → same proposer
//   • upstream commitment hash missing from DRACHMA commitment
//   • missing upstream_commitment_hash encoding check

#include "common/commitments.h"
#include "layer1-talanton/tx/l1_commitment_validator.h"
#include "layer2-drachma/consensus/pos_consensus.h"
#include "layer3-obolos/consensus/pos_consensus.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace pantheon;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string hex64(char c)
{
    return std::string(64, c);
}

// Build a minimal valid OBOLOS commitment.
static common::Commitment make_valid_l3_commit(uint64_t epoch, uint64_t height,
                                               std::vector<common::FinalitySignature> sigs)
{
    return obolos::BuildL3Commitment(
        epoch, height,
        hex64('a'),  // finalized_block_hash
        hex64('b'),  // state_root
        hex64('c'),  // validator_set_hash
        std::move(sigs));
}

// Build a minimal valid DRACHMA commitment (for TALANTON anchoring).
static common::Commitment make_valid_l2_commit(uint64_t epoch, uint64_t height,
                                               std::vector<common::FinalitySignature> sigs)
{
    return common::Commitment{
        common::SourceChain::DRACHMA,
        epoch,
        height,
        hex64('d'),  // finalized_block_hash
        hex64('e'),  // state_root
        hex64('f'),  // validator_set_hash
        hex64('9'),  // upstream_commitment_hash (required for DRACHMA)
        std::move(sigs)};
}

// ---------------------------------------------------------------------------
// Phase B — L3 (OBOLOS) finality validation
// ---------------------------------------------------------------------------

void test_l3_valid_finality() {
    std::cout << "[consensus] L3 valid finality accepted" << std::endl;

    // Two validators: total stake 100, signed stake 70 + 34 = 104 → ≥ 2/3 of active_stake=150.
    auto commit = make_valid_l3_commit(1, 10, {{"v1", 70, "sig1"}, {"v2", 34, "sig2"}});
    auto result = obolos::ValidateL3Finality(commit, /*last=*/5, /*active_stake=*/150);
    assert(result.valid);

    std::cout << "  ✓ Valid L3 finality accepted." << std::endl;
}

void test_l3_height_replay() {
    std::cout << "[consensus] L3 height replay rejected (non-monotonic)" << std::endl;

    auto commit = make_valid_l3_commit(1, 10, {{"v1", 70, "sig1"}, {"v2", 34, "sig2"}});

    // last_finalized_height == finalized_height → replay.
    auto r1 = obolos::ValidateL3Finality(commit, 10, 150);
    assert(!r1.valid);

    // last_finalized_height > finalized_height → replay.
    auto r2 = obolos::ValidateL3Finality(commit, 11, 150);
    assert(!r2.valid);

    std::cout << "  ✓ L3 height replay correctly rejected." << std::endl;
}

void test_l3_forged_source_chain() {
    std::cout << "[consensus] L3 commitment with forged source chain rejected" << std::endl;

    auto commit = make_valid_l3_commit(1, 10, {{"v1", 70, "sig1"}, {"v2", 34, "sig2"}});
    // Change source chain to DRACHMA — should be rejected.
    commit.source_chain = common::SourceChain::DRACHMA;
    auto result = obolos::ValidateL3Finality(commit, 5, 150);
    assert(!result.valid);

    std::cout << "  ✓ Forged source chain rejected by L3 validator." << std::endl;
}

void test_l3_insufficient_quorum() {
    std::cout << "[consensus] L3 insufficient quorum rejected (< 2/3)" << std::endl;

    // 40% of 150 active stake signed — well below 2/3.
    auto commit = make_valid_l3_commit(1, 10, {{"v1", 40, "sig1"}, {"v2", 20, "sig2"}});
    auto result = obolos::ValidateL3Finality(commit, 5, 150);
    assert(!result.valid);

    // Exactly at 2/3 boundary: signed = 100, active = 150, 100 * 3 == 300 >= 150 * 2 == 300 → passes.
    auto commit_boundary = make_valid_l3_commit(1, 10, {{"v1", 100, "sig1"}});
    auto r_boundary = obolos::ValidateL3Finality(commit_boundary, 5, 150);
    assert(r_boundary.valid);

    // One below boundary: signed = 99, 99 * 3 = 297 < 300 → fails.
    auto commit_below = make_valid_l3_commit(1, 10, {{"v1", 99, "sig1"}});
    auto r_below = obolos::ValidateL3Finality(commit_below, 5, 150);
    assert(!r_below.valid);

    std::cout << "  ✓ Insufficient quorum rejected; boundary cases correct." << std::endl;
}

void test_l3_duplicate_signer() {
    std::cout << "[consensus] L3 duplicate signer stake counted only once" << std::endl;

    // Same validator_id "v1" signs twice — stake should only be counted once.
    auto commit = make_valid_l3_commit(1, 10, {{"v1", 70, "sig1"}, {"v1", 70, "sig_dup"}});

    // Total signed stake after dedup = 70. active_stake = 150. 70/150 < 2/3 → should FAIL.
    auto result = obolos::ValidateL3Finality(commit, 5, 150);
    assert(!result.valid);

    // Confirm that 70 is the deduplicated weight.
    assert(common::SignedStakeWeight(commit) == 70);

    std::cout << "  ✓ Duplicate signer detected; stake counted once only." << std::endl;
}

void test_l3_zero_active_stake() {
    std::cout << "[consensus] L3 zero active stake rejected" << std::endl;

    auto commit = make_valid_l3_commit(1, 10, {{"v1", 70, "sig1"}});
    auto result = obolos::ValidateL3Finality(commit, 5, 0);
    assert(!result.valid);

    std::cout << "  ✓ Zero active stake correctly rejected." << std::endl;
}

void test_l3_empty_signatures() {
    std::cout << "[consensus] L3 empty signature set rejected" << std::endl;

    auto commit = make_valid_l3_commit(1, 10, {});
    auto result = obolos::ValidateL3Finality(commit, 5, 150);
    assert(!result.valid);

    std::cout << "  ✓ Empty signature set correctly rejected." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase B — L2 (DRACHMA) commitment anchoring to L1 (TALANTON)
// ---------------------------------------------------------------------------

void test_l2_valid_commit() {
    std::cout << "[consensus] L2 valid commit accepted by TALANTON" << std::endl;

    auto commit = make_valid_l2_commit(1, 20, {{"v3", 80, "sig3"}, {"v4", 30, "sig4"}});
    auto result = talanton::ValidateL2Commit(commit, {10}, 160);
    assert(result.valid);

    std::cout << "  ✓ Valid L2 commit accepted by TALANTON." << std::endl;
}

void test_l2_height_replay() {
    std::cout << "[consensus] L2 height replay rejected by TALANTON" << std::endl;

    auto commit = make_valid_l2_commit(1, 20, {{"v3", 80, "sig3"}, {"v4", 30, "sig4"}});

    // same height as last_finalized_height → replay.
    auto r1 = talanton::ValidateL2Commit(commit, {20}, 160);
    assert(!r1.valid);

    // height behind → replay.
    auto r2 = talanton::ValidateL2Commit(commit, {21}, 160);
    assert(!r2.valid);

    std::cout << "  ✓ L2 height replay rejected by TALANTON." << std::endl;
}

void test_l2_forged_source_chain() {
    std::cout << "[consensus] L2 commitment with forged source chain rejected" << std::endl;

    auto commit = make_valid_l2_commit(1, 20, {{"v3", 80, "sig3"}});
    // Change to OBOLOS — TALANTON validator expects DRACHMA.
    commit.source_chain = common::SourceChain::OBOLOS;
    auto result = talanton::ValidateL2Commit(commit, {10}, 160);
    assert(!result.valid);

    std::cout << "  ✓ Forged source chain rejected by TALANTON." << std::endl;
}

void test_l2_missing_upstream_commitment_hash() {
    std::cout << "[consensus] L2 commit missing upstream_commitment_hash rejected" << std::endl;

    // DRACHMA commitment with empty upstream_commitment_hash should fail encoding validation.
    common::Commitment bad_commit{
        common::SourceChain::DRACHMA,
        1,
        20,
        hex64('d'),  // finalized_block_hash
        hex64('e'),  // state_root
        hex64('f'),  // validator_set_hash
        "",          // upstream_commitment_hash MISSING
        {{"v3", 80, "sig3"}}};

    auto result = talanton::ValidateL2Commit(bad_commit, {10}, 160);
    assert(!result.valid);

    std::cout << "  ✓ DRACHMA commit without upstream_commitment_hash rejected." << std::endl;
}

void test_l2_insufficient_quorum() {
    std::cout << "[consensus] L2 insufficient quorum rejected by TALANTON" << std::endl;

    // Only 30% of active stake signed.
    auto commit = make_valid_l2_commit(1, 20, {{"v3", 30, "sig3"}});
    auto result = talanton::ValidateL2Commit(commit, {10}, 150);
    assert(!result.valid);

    std::cout << "  ✓ L2 insufficient quorum rejected by TALANTON." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase B — DRACHMA ValidateL3Commit path
// ---------------------------------------------------------------------------

void test_drachma_validate_l3_commit() {
    std::cout << "[consensus] DRACHMA ValidateL3Commit: valid OBOLOS commit accepted" << std::endl;

    auto commit = make_valid_l3_commit(1, 10, {{"v1", 100, "sig1"}});
    auto result = drachma::ValidateL3Commit(commit, /*last_l3_height=*/5, /*active_stake=*/150);
    assert(result.valid);

    // Wrong source chain.
    auto bad = commit;
    bad.source_chain = common::SourceChain::DRACHMA;
    assert(!drachma::ValidateL3Commit(bad, 5, 150).valid);

    std::cout << "  ✓ DRACHMA ValidateL3Commit works correctly." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase B — Slashing
// ---------------------------------------------------------------------------

void test_slashing_equivocation() {
    std::cout << "[slashing] Equivocation: 10% slash (1000 bips)" << std::endl;

    drachma::Validator v{"bad-val", 100000};
    auto ev = drachma::ApplySlashing(v, "equivocation");
    assert(ev.slashed_amount == 10000);
    assert(v.stake == 90000);

    std::cout << "  ✓ Equivocation slash: 10000 slashed, 90000 remaining." << std::endl;
}

void test_slashing_double_sign() {
    std::cout << "[slashing] Double sign: 5% slash (500 bips)" << std::endl;

    drachma::Validator v{"double-signer", 100000};
    auto ev = drachma::ApplySlashing(v, "double_sign");
    assert(ev.slashed_amount == 5000);
    assert(v.stake == 95000);

    // OBOLOS slashing should match.
    obolos::Validator ov{"bad-obl-val", 100000};
    auto oev = obolos::ApplySlashing(ov, "double_sign");
    assert(oev.slashed_amount == 5000);
    assert(ov.stake == 95000);

    std::cout << "  ✓ Double-sign slash: 5000 slashed, 95000 remaining." << std::endl;
}

void test_slashing_unknown_reason() {
    std::cout << "[slashing] Unknown slashing reason throws exception" << std::endl;

    drachma::Validator v{"bad-val", 1000};
    bool threw = false;
    try {
        drachma::ApplySlashing(v, "bribery");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    std::cout << "  ✓ Unknown slashing reason throws std::invalid_argument." << std::endl;
}

void test_slashing_rounding() {
    std::cout << "[slashing] Slashing integer truncation (rounding)" << std::endl;

    // stake = 1 → 10% of 1 = 0 (integer truncation, rounds down)
    drachma::Validator v{"tiny", 1};
    auto ev = drachma::ApplySlashing(v, "equivocation");
    assert(ev.slashed_amount == 0);
    assert(v.stake == 1);  // No underflow

    // stake = 9 → 10% of 9 = 0 (rounds down)
    drachma::Validator v2{"small", 9};
    auto ev2 = drachma::ApplySlashing(v2, "equivocation");
    assert(ev2.slashed_amount == 0);
    assert(v2.stake == 9);

    // stake = 10 → 10% = 1
    drachma::Validator v3{"ten", 10};
    auto ev3 = drachma::ApplySlashing(v3, "equivocation");
    assert(ev3.slashed_amount == 1);
    assert(v3.stake == 9);

    std::cout << "  ✓ Slashing truncation correct; stake never goes negative." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase B — Proposer Determinism
// ---------------------------------------------------------------------------

void test_proposer_determinism() {
    std::cout << "[consensus] Proposer selection is deterministic" << std::endl;

    std::vector<drachma::Validator> validators{{"v1", 80}, {"v2", 20}};

    // Same epoch/height must yield the same proposer.
    const auto& p1 = drachma::SelectDeterministicProposer(validators, 3, 22);
    const auto& p2 = drachma::SelectDeterministicProposer(validators, 3, 22);
    assert(p1.id == p2.id);

    // Different epoch should generally differ (not guaranteed by all inputs, but deterministic).
    const auto& p3 = drachma::SelectDeterministicProposer(validators, 4, 22);
    const auto& p4 = drachma::SelectDeterministicProposer(validators, 4, 22);
    assert(p3.id == p4.id);

    // OBOLOS proposer determinism.
    std::vector<obolos::Validator> ov{{"a", 60}, {"b", 40}};
    const auto& op1 = obolos::SelectDeterministicProposer(ov, 7, 99);
    const auto& op2 = obolos::SelectDeterministicProposer(ov, 7, 99);
    assert(op1.id == op2.id);

    std::cout << "  ✓ Proposer selection is deterministic." << std::endl;
}

void test_proposer_empty_set_throws() {
    std::cout << "[consensus] Proposer selection with empty set throws" << std::endl;

    std::vector<drachma::Validator> empty{};
    bool threw = false;
    try {
        drachma::SelectDeterministicProposer(empty, 1, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    std::cout << "  ✓ Empty validator set throws std::invalid_argument." << std::endl;
}

// ---------------------------------------------------------------------------
// Phase K — Encoding invariants
// ---------------------------------------------------------------------------

void test_payload_encoding_validation() {
    std::cout << "[encoding] Invalid hex/length in commitment fields rejected" << std::endl;

    // Short block hash (not 64 chars) must fail encoding validation.
    common::Commitment bad1{
        common::SourceChain::OBOLOS, 1, 10,
        "abc",           // too short
        hex64('b'), hex64('c'), "", {{"v1", 70, "sig"}}};
    assert(!common::ValidatePayloadEncoding(bad1).valid);

    // Non-hex character in state_root.
    common::Commitment bad2{
        common::SourceChain::OBOLOS, 1, 10,
        hex64('a'),
        std::string(63, 'b') + "Z",  // Z is not hex
        hex64('c'), "", {{"v1", 70, "sig"}}};
    assert(!common::ValidatePayloadEncoding(bad2).valid);

    // Zero finalized_height.
    common::Commitment bad3{
        common::SourceChain::OBOLOS, 1, 0, hex64('a'), hex64('b'), hex64('c'), "",
        {{"v1", 70, "sig"}}};
    assert(!common::ValidatePayloadEncoding(bad3).valid);

    // Empty validator_id in signature.
    common::Commitment bad4{
        common::SourceChain::OBOLOS, 1, 10, hex64('a'), hex64('b'), hex64('c'), "",
        {{"", 70, "sig"}}};
    assert(!common::ValidatePayloadEncoding(bad4).valid);

    std::cout << "  ✓ Malformed commitment payloads are rejected at encoding level." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== PantheonChain Adversarial Consensus Tests (Phase B) ===" << std::endl;
    std::cout << std::endl;

    // L3 finality
    test_l3_valid_finality();
    test_l3_height_replay();
    test_l3_forged_source_chain();
    test_l3_insufficient_quorum();
    test_l3_duplicate_signer();
    test_l3_zero_active_stake();
    test_l3_empty_signatures();

    // L2 anchoring to L1
    test_l2_valid_commit();
    test_l2_height_replay();
    test_l2_forged_source_chain();
    test_l2_missing_upstream_commitment_hash();
    test_l2_insufficient_quorum();

    // DRACHMA ValidateL3Commit path
    test_drachma_validate_l3_commit();

    // Slashing
    test_slashing_equivocation();
    test_slashing_double_sign();
    test_slashing_unknown_reason();
    test_slashing_rounding();

    // Proposer determinism
    test_proposer_determinism();
    test_proposer_empty_set_throws();

    // Encoding invariants
    test_payload_encoding_validation();

    std::cout << std::endl;
    std::cout << "✓ All adversarial consensus tests passed." << std::endl;
    return 0;
}
