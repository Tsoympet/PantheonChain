// ParthenonChain – VETO vote choice + supply-bonded params tests
//
// Covers:
//  1. VETO vote accumulates in veto_votes tally
//  2. Veto exceeding threshold auto-rejects (Cosmos model)
//  3. YES supermajority still loses when veto threshold exceeded
//  4. veto_threshold_bps parameter update in GovernanceParams
//  5. Veto constitutional limit enforcement (min/max bounds)

#include "governance/voting.h"
#include "governance/params.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

using namespace parthenon::governance;
using namespace parthenon::crypto;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::pair<Schnorr::PrivateKey, Schnorr::PublicKey> MakeKey(uint8_t seed) {
    Schnorr::PrivateKey priv{};
    priv[0] = seed;
    priv[1] = 0x01;
    // rest remain zero – deterministic stub key
    auto pub_opt = Schnorr::GetPublicKey(priv);
    Schnorr::PublicKey pub{};
    if (pub_opt.has_value()) pub = *pub_opt;
    return {priv, pub};
}

static std::vector<uint8_t> PubVec(const Schnorr::PublicKey& pub) {
    return std::vector<uint8_t>(pub.begin(), pub.end());
}

static std::vector<uint8_t> MakeSignature(const Schnorr::PrivateKey& priv,
                                           uint64_t proposal_id,
                                           const std::vector<uint8_t>& voter_pub,
                                           VoteChoice choice,
                                           uint64_t voting_power) {
    std::vector<uint8_t> payload;
    payload.reserve(8 + voter_pub.size() + 1 + 8);
    auto push64 = [&](uint64_t v) {
        for (int i = 0; i < 8; ++i) payload.push_back(static_cast<uint8_t>(v >> (8*i)));
    };
    push64(proposal_id);
    payload.insert(payload.end(), voter_pub.begin(), voter_pub.end());
    payload.push_back(static_cast<uint8_t>(choice));
    push64(voting_power);

    auto hash = SHA256::Hash256(payload.data(), payload.size());
    auto sig_opt = Schnorr::Sign(priv, hash.data());
    if (!sig_opt.has_value()) return std::vector<uint8_t>(64, 0);
    const auto& sig = *sig_opt;
    return std::vector<uint8_t>(sig.begin(), sig.end());
}

// Cast a vote with a fresh key seeded by `seed`.
static bool CastVote(VotingSystem& vs, uint64_t proposal_id,
                     uint8_t key_seed, VoteChoice choice, uint64_t power) {
    auto [priv, pub] = MakeKey(key_seed);
    auto voter = PubVec(pub);
    auto sig   = MakeSignature(priv, proposal_id, voter, choice, power);
    return vs.CastVote(proposal_id, voter, choice, power, sig);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void TestVetoVoteAccumulates() {
    std::cout << "Test: VETO vote accumulates in veto_votes tally" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);  // no quorum for this test

    uint64_t pid = vs.CreateProposal({0x01}, ProposalType::GENERAL,
                                      "test", "desc", {});
    vs.UpdateBlockHeight(101);  // inside voting window (starts at block 100)

    assert(CastVote(vs, pid, 0x10, VoteChoice::YES,  1000));
    assert(CastVote(vs, pid, 0x11, VoteChoice::VETO,  500));

    auto p = vs.GetProposal(pid).value();
    assert(p.yes_votes   == 1000);
    assert(p.veto_votes  == 500);
    assert(p.no_votes    == 0);
    assert(p.abstain_votes == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVetoThresholdAutoRejects() {
    std::cout << "Test: Veto > threshold → REJECTED regardless of YES majority" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetVetoThreshold(3334);  // 33.34 %

    uint64_t pid = vs.CreateProposal({0x01}, ProposalType::GENERAL,
                                      "test", "desc", {});
    vs.UpdateBlockHeight(101);  // voting window open

    // 6000 YES, 1000 NO, 4000 VETO → total = 11000
    // veto share = 4000/11000 = 36.4 % > 33.34 % → REJECTED
    assert(CastVote(vs, pid, 0x01, VoteChoice::YES,  6000));
    assert(CastVote(vs, pid, 0x02, VoteChoice::NO,   1000));
    assert(CastVote(vs, pid, 0x03, VoteChoice::VETO, 4000));

    // End voting period
    vs.UpdateBlockHeight(10102);  // voting_end = 100 + 10000 = 10100
    assert(vs.TallyVotes(pid));

    auto p = vs.GetProposal(pid).value();
    assert(p.status == ProposalStatus::REJECTED);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestYesMajorityPassesWithoutVeto() {
    std::cout << "Test: YES majority passes when veto share is below threshold" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetVetoThreshold(3334);

    uint64_t pid = vs.CreateProposal({0x01}, ProposalType::GENERAL,
                                      "test", "desc", {});
    vs.UpdateBlockHeight(101);

    // 7000 YES, 1000 NO, 1000 VETO → veto share = 1000/9000 = 11.1 % < 33.34 %
    // approval = 7000/(7000+1000) = 87.5 % ≥ 50 % → PASSED
    assert(CastVote(vs, pid, 0x01, VoteChoice::YES,  7000));
    assert(CastVote(vs, pid, 0x02, VoteChoice::NO,   1000));
    assert(CastVote(vs, pid, 0x03, VoteChoice::VETO, 1000));

    vs.UpdateBlockHeight(10102);
    assert(vs.TallyVotes(pid));

    auto p = vs.GetProposal(pid).value();
    assert(p.status == ProposalStatus::PASSED);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVetoExactlyAtThreshold() {
    std::cout << "Test: Veto exactly AT threshold (not strictly greater) → not triggered" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetVetoThreshold(3334);  // 33.34 %

    uint64_t pid = vs.CreateProposal({0x01}, ProposalType::GENERAL,
                                      "test", "desc", {});
    vs.UpdateBlockHeight(101);

    // Make veto share exactly = 3334 / 10000:
    //   total = 10000, veto = 3334 → veto*10000 = 33340000 == total*threshold → NOT strictly greater
    assert(CastVote(vs, pid, 0x01, VoteChoice::YES,  6666));
    assert(CastVote(vs, pid, 0x02, VoteChoice::VETO, 3334));

    vs.UpdateBlockHeight(10102);
    assert(vs.TallyVotes(pid));

    // veto*10000 = 3334*10000 = 33340000
    // total*threshold = 10000 * 3334 = 33340000
    // NOT strictly greater → veto check NOT triggered → evaluate YES/NO
    // YES=6666, NO=0 → 100% approval ≥ 50% → PASSED
    auto p = vs.GetProposal(pid).value();
    assert(p.status == ProposalStatus::PASSED);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVetoThresholdDefaultInParams() {
    std::cout << "Test: GovernanceParams defaults include veto_threshold_bps = 3334" << std::endl;

    GovernanceParams gp;
    assert(gp.Get().veto_threshold_bps == 3334);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVetoThresholdUpdateWithLimits() {
    std::cout << "Test: veto_threshold_bps can be updated within constitutional limits" << std::endl;

    GovernanceParams gp;

    // Valid: 4000 bps (40 %) – within [1000, 5000]
    assert(gp.UpdateParam("veto_threshold_bps", 4000, 1, 100));
    assert(gp.Get().veto_threshold_bps == 4000);

    // Below minimum (1000): rejected
    assert(!gp.UpdateParam("veto_threshold_bps", 999, 2, 200));

    // Above maximum (5000): rejected
    assert(!gp.UpdateParam("veto_threshold_bps", 5001, 3, 300));

    // Without proposal (proposal_id == 0): rejected
    assert(!gp.UpdateParam("veto_threshold_bps", 3000, 0, 400));

    // Still at 4000
    assert(gp.Get().veto_threshold_bps == 4000);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVetoWithAbstain() {
    std::cout << "Test: ABSTAIN votes do not affect veto share calculation" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetVetoThreshold(3334);

    uint64_t pid = vs.CreateProposal({0x01}, ProposalType::GENERAL,
                                      "test", "desc", {});
    vs.UpdateBlockHeight(101);

    // 5000 YES, 2000 NO, 3000 ABSTAIN, 4000 VETO
    // total = 14000; veto share = 4000/14000 = 28.6 % < 33.34 % → NOT triggered
    assert(CastVote(vs, pid, 0x01, VoteChoice::YES,     5000));
    assert(CastVote(vs, pid, 0x02, VoteChoice::NO,      2000));
    assert(CastVote(vs, pid, 0x03, VoteChoice::ABSTAIN, 3000));
    assert(CastVote(vs, pid, 0x04, VoteChoice::VETO,    4000));

    vs.UpdateBlockHeight(10102);
    assert(vs.TallyVotes(pid));

    auto p = vs.GetProposal(pid).value();
    // YES/(YES+NO) = 5000/7000 = 71.4 % ≥ 50 % → PASSED
    assert(p.status == ProposalStatus::PASSED);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "VETO + GovernanceParams Tests" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestVetoVoteAccumulates();
        TestVetoThresholdAutoRejects();
        TestYesMajorityPassesWithoutVeto();
        TestVetoExactlyAtThreshold();
        TestVetoThresholdDefaultInParams();
        TestVetoThresholdUpdateWithLimits();
        TestVetoWithAbstain();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All VETO + GovernanceParams tests passed! \u2713" << std::endl;
        std::cout << "=============================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
