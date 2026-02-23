// ParthenonChain – VotingSystem integration tests
// Tests anti-whale scaling, Boule screening, proposal deposits,
// CONSTITUTIONAL / EMERGENCY proposal types.

#include "governance/voting.h"
#include "governance/antiwhale.h"
#include "governance/boule.h"
#include "core/crypto/schnorr.h"
#include "core/crypto/sha256.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace parthenon::governance;
using namespace parthenon::crypto;

static constexpr uint64_t VOTING_START_DELAY = 100;
static constexpr uint64_t VOTING_PERIOD      = 10000;

static Schnorr::Signature MakeVoteSignature(const Schnorr::PrivateKey& priv,
                                            uint64_t pid,
                                            const Schnorr::PublicKey& pub,
                                            VoteChoice choice,
                                            uint64_t power) {
    std::vector<uint8_t> payload;
    payload.reserve(8 + pub.size() + 1 + 8);
    auto le64 = [&](uint64_t v) {
        for (int i = 0; i < 8; ++i)
            payload.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    };
    le64(pid);
    payload.insert(payload.end(), pub.begin(), pub.end());
    payload.push_back(static_cast<uint8_t>(choice));
    le64(power);
    auto hash = SHA256::Hash256(payload.data(), payload.size());
    return *Schnorr::Sign(priv, hash.data(), nullptr);
}

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

// ---------------------------------------------------------------------------
// Anti-whale integration
// ---------------------------------------------------------------------------

void TestAntiWhaleScalesVotingPower() {
    std::cout << "Test: Anti-whale guard scales effective voting power in tallies" << std::endl;

    AntiWhaleGuard::Config cfg;
    cfg.quadratic_voting_enabled = true;
    cfg.max_voting_power_cap     = 0;
    cfg.whale_threshold_bps      = 0;
    AntiWhaleGuard guard(cfg);

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetAntiWhaleGuard(&guard);
    vs.SetTotalSupply(1000000);

    Schnorr::PrivateKey priv{};
    priv[31] = 0x11;
    auto pub = *Schnorr::GetPublicKey(priv);
    std::vector<uint8_t> voter(pub.begin(), pub.end());

    uint64_t id = vs.CreateProposal(Addr(0x01), ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    // Raw power = 10000 → effective = sqrt(10000) = 100
    uint64_t raw_power = 10000;
    auto sig = MakeVoteSignature(priv, id, pub, VoteChoice::YES, raw_power);
    assert(vs.CastVote(id, voter, VoteChoice::YES, raw_power,
                       std::vector<uint8_t>(sig.begin(), sig.end())));

    vs.UpdateBlockHeight(VOTING_START_DELAY + VOTING_PERIOD + 1);
    vs.TallyVotes(id);

    auto p = vs.GetProposal(id);
    assert(p.has_value());
    // Tally should record effective power (100), not raw (10000)
    assert(p->yes_votes == 100);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestAntiWhaleDetached() {
    std::cout << "Test: Without anti-whale guard raw power is used" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    // No guard attached

    Schnorr::PrivateKey priv{};
    priv[31] = 0x22;
    auto pub = *Schnorr::GetPublicKey(priv);
    std::vector<uint8_t> voter(pub.begin(), pub.end());

    uint64_t id = vs.CreateProposal(Addr(0x01), ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    uint64_t raw_power = 10000;
    auto sig = MakeVoteSignature(priv, id, pub, VoteChoice::YES, raw_power);
    assert(vs.CastVote(id, voter, VoteChoice::YES, raw_power,
                       std::vector<uint8_t>(sig.begin(), sig.end())));

    vs.UpdateBlockHeight(VOTING_START_DELAY + VOTING_PERIOD + 1);
    vs.TallyVotes(id);

    auto p = vs.GetProposal(id);
    assert(p.has_value() && p->yes_votes == raw_power);

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Boule screening integration
// ---------------------------------------------------------------------------

void TestBouleScreeningBlocksVoting() {
    std::cout << "Test: Boule screening blocks voting until approved" << std::endl;

    Boule boule(3, 1000, 0, /*screening=*/true);
    for (uint8_t i = 1; i <= 5; ++i) boule.RegisterCitizen(Addr(i), 1, 0);
    std::vector<uint8_t> seed = {0x0A, 0x0B, 0x0C, 0x0D};
    boule.ConductSortition(seed, 0);

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetBoule(&boule);
    vs.SetRequireBouleApproval(true);

    Schnorr::PrivateKey priv{};
    priv[31] = 0x33;
    auto pub = *Schnorr::GetPublicKey(priv);
    std::vector<uint8_t> voter(pub.begin(), pub.end());

    uint64_t id = vs.CreateProposal(Addr(0x01), ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    auto sig = MakeVoteSignature(priv, id, pub, VoteChoice::YES, 1);
    // No Boule approval yet → vote must be rejected
    assert(!vs.CastVote(id, voter, VoteChoice::YES, 1,
                        std::vector<uint8_t>(sig.begin(), sig.end())));

    // Approve via Boule (need 2/3 of 3 = 2 approvals)
    auto council = boule.GetCurrentCouncil();
    boule.ReviewProposal(id, council[0].address, true, "ok", 0);
    boule.ReviewProposal(id, council[1].address, true, "ok", 0);

    // Now vote should succeed (Boule approval detected via boule_ pointer)
    assert(vs.CastVote(id, voter, VoteChoice::YES, 1,
                       std::vector<uint8_t>(sig.begin(), sig.end())));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestMarkBouleApproved() {
    std::cout << "Test: MarkBouleApproved enables voting without Boule object" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetRequireBouleApproval(true);
    // No Boule object attached – must use explicit MarkBouleApproved()

    Schnorr::PrivateKey priv{};
    priv[31] = 0x44;
    auto pub = *Schnorr::GetPublicKey(priv);
    std::vector<uint8_t> voter(pub.begin(), pub.end());

    uint64_t id = vs.CreateProposal(Addr(0x02), ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    auto sig = MakeVoteSignature(priv, id, pub, VoteChoice::NO, 1);
    assert(!vs.CastVote(id, voter, VoteChoice::NO, 1,
                        std::vector<uint8_t>(sig.begin(), sig.end())));

    assert(vs.MarkBouleApproved(id));
    assert(vs.CastVote(id, voter, VoteChoice::NO, 1,
                       std::vector<uint8_t>(sig.begin(), sig.end())));

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Proposal types
// ---------------------------------------------------------------------------

void TestConstitutionalProposalHigherThreshold() {
    std::cout << "Test: CONSTITUTIONAL proposal uses 66% threshold" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    vs.SetDefaultThreshold(50);

    std::vector<uint8_t> proposer(32, 0x01);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::CONSTITUTIONAL,
                                    "Change constitution", "Desc", {});
    auto p = vs.GetProposal(id);
    assert(p.has_value());
    assert(p->approval_threshold == 66);

    // GENERAL proposal still uses default threshold
    uint64_t id2 = vs.CreateProposal(proposer, ProposalType::GENERAL, "G", "D", {});
    auto p2 = vs.GetProposal(id2);
    assert(p2.has_value() && p2->approval_threshold == 50);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestEmergencyProposalType() {
    std::cout << "Test: EMERGENCY proposal type is created correctly" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    std::vector<uint8_t> proposer(32, 0x02);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::EMERGENCY,
                                    "Security patch", "CVE-XXXX", {});
    auto p = vs.GetProposal(id);
    assert(p.has_value() && p->type == ProposalType::EMERGENCY);

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Proposal deposit
// ---------------------------------------------------------------------------

void TestProposalDeposit() {
    std::cout << "Test: Proposal deposit tracking" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(0);
    std::vector<uint8_t> proposer(32, 0x03);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::GENERAL,
                                    "T", "D", {}, /*deposit=*/1000);

    auto p = vs.GetProposal(id);
    assert(p.has_value());
    assert(p->deposit_amount   == 1000);
    assert(!p->deposit_returned);

    // Return deposit
    assert(vs.ReturnDeposit(id));
    p = vs.GetProposal(id);
    assert(p->deposit_returned);

    // Cannot return twice
    assert(!vs.ReturnDeposit(id));

    // Slash deposit on a different proposal
    uint64_t id2 = vs.CreateProposal(proposer, ProposalType::GENERAL,
                                     "T2", "D2", {}, /*deposit=*/500);
    assert(vs.SlashDeposit(id2));
    p = vs.GetProposal(id2);
    assert(p->deposit_returned);

    // Cannot slash twice
    assert(!vs.SlashDeposit(id2));

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "VotingSystem Integration Test Suite" << std::endl;
    std::cout << "(Anti-whale, Boule, Deposits, New Types)" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestAntiWhaleScalesVotingPower();
        TestAntiWhaleDetached();
        TestBouleScreeningBlocksVoting();
        TestMarkBouleApproved();
        TestConstitutionalProposalHigherThreshold();
        TestEmergencyProposalType();
        TestProposalDeposit();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All VotingSystem integration tests passed! \u2713" << std::endl;
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
