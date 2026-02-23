// ParthenonChain - Governance Module Unit Tests
// Tests VotingSystem, TreasuryManager, and DelegationSystem

#include "governance/voting.h"
#include "core/crypto/schnorr.h"
#include "core/crypto/sha256.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace parthenon::governance;
using namespace parthenon::crypto;

// Constants matching VotingSystem defaults (see voting.cpp)
static constexpr uint64_t VOTING_START_DELAY = 100;   // blocks before voting opens
static constexpr uint64_t VOTING_PERIOD      = 10000; // default voting period in blocks
static constexpr uint64_t NO_QUORUM_REQUIRED = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build canonical vote payload and sign it (mirrors CastVote internals).
static Schnorr::Signature MakeVoteSignature(const Schnorr::PrivateKey& privkey,
                                            uint64_t proposal_id,
                                            const Schnorr::PublicKey& voter,
                                            VoteChoice choice,
                                            uint64_t voting_power) {
    std::vector<uint8_t> payload;
    payload.reserve(8 + voter.size() + 1 + 8);

    auto append_u64_le = [&payload](uint64_t value) {
        for (size_t i = 0; i < 8; ++i) {
            payload.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
        }
    };

    append_u64_le(proposal_id);
    payload.insert(payload.end(), voter.begin(), voter.end());
    payload.push_back(static_cast<uint8_t>(choice));
    append_u64_le(voting_power);

    auto hash = SHA256::Hash256(payload.data(), payload.size());
    auto sig_opt = Schnorr::Sign(privkey, hash.data(), nullptr);
    assert(sig_opt.has_value());
    return *sig_opt;
}

// ---------------------------------------------------------------------------
// VotingSystem Tests
// ---------------------------------------------------------------------------

void TestCreateProposal() {
    std::cout << "Test: CreateProposal" << std::endl;

    VotingSystem vs;
    std::vector<uint8_t> proposer(32, 0xAB);
    std::vector<uint8_t> exec_data = {0x01, 0x02};

    uint64_t id = vs.CreateProposal(proposer, ProposalType::PARAMETER_CHANGE,
                                    "Test Proposal", "Description", exec_data);
    assert(id == 1);

    auto proposal_opt = vs.GetProposal(id);
    assert(proposal_opt.has_value());

    const Proposal& p = *proposal_opt;
    assert(p.proposal_id == 1);
    assert(p.type == ProposalType::PARAMETER_CHANGE);
    assert(p.status == ProposalStatus::PENDING);
    assert(p.title == "Test Proposal");
    assert(p.description == "Description");
    assert(p.proposer == proposer);
    assert(p.approval_threshold == 50);

    // Second proposal gets the next ID
    uint64_t id2 = vs.CreateProposal(proposer, ProposalType::GENERAL,
                                     "Second", "Desc2", {});
    assert(id2 == 2);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetProposalNotFound() {
    std::cout << "Test: GetProposal (not found)" << std::endl;

    VotingSystem vs;
    assert(!vs.GetProposal(999).has_value());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestCastVoteAndTally() {
    std::cout << "Test: CastVote and TallyVotes" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(NO_QUORUM_REQUIRED);

    // Generate a key pair for the voter
    Schnorr::PrivateKey privkey{};
    privkey[31] = 0x05;
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());
    const Schnorr::PublicKey& pubkey = *pubkey_opt;

    std::vector<uint8_t> proposer(32, 0x01);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::GENERAL,
                                    "Vote Test", "Desc", {});

    // Advance block height past voting_start (proposal created at block 0,
    // voting_start = VOTING_START_DELAY, so move one block past it).
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    std::vector<uint8_t> voter(pubkey.begin(), pubkey.end());
    uint64_t power = 100;
    auto sig = MakeVoteSignature(privkey, id, pubkey, VoteChoice::YES, power);
    std::vector<uint8_t> sig_bytes(sig.begin(), sig.end());

    bool ok = vs.CastVote(id, voter, VoteChoice::YES, power, sig_bytes);
    assert(ok);

    // Duplicate vote must be rejected
    bool dup = vs.CastVote(id, voter, VoteChoice::YES, power, sig_bytes);
    assert(!dup);

    // HasVoted should return true
    assert(vs.HasVoted(id, voter));

    // Tally: move past voting_end (voting_start=VOTING_START_DELAY, period=VOTING_PERIOD)
    vs.UpdateBlockHeight(VOTING_START_DELAY + VOTING_PERIOD + 1);
    bool tally_ok = vs.TallyVotes(id);
    assert(tally_ok);

    auto updated = vs.GetProposal(id);
    assert(updated.has_value());
    // 100% yes votes -> PASSED
    assert(updated->status == ProposalStatus::PASSED);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestCastVoteBeforeVotingStart() {
    std::cout << "Test: CastVote rejected before voting_start" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(NO_QUORUM_REQUIRED);

    Schnorr::PrivateKey privkey{};
    privkey[31] = 0x07;
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());
    const Schnorr::PublicKey& pubkey = *pubkey_opt;

    std::vector<uint8_t> proposer(32, 0x02);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::GENERAL, "T", "D", {});

    // Block height is 0; voting_start = 100, so vote should be rejected
    std::vector<uint8_t> voter(pubkey.begin(), pubkey.end());
    auto sig = MakeVoteSignature(privkey, id, pubkey, VoteChoice::YES, 10);
    std::vector<uint8_t> sig_bytes(sig.begin(), sig.end());

    bool ok = vs.CastVote(id, voter, VoteChoice::YES, 10, sig_bytes);
    assert(!ok);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVoteRejectedWithBadSignature() {
    std::cout << "Test: CastVote rejected with bad signature" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(NO_QUORUM_REQUIRED);

    Schnorr::PrivateKey privkey{};
    privkey[31] = 0x09;
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());
    const Schnorr::PublicKey& pubkey = *pubkey_opt;

    std::vector<uint8_t> proposer(32, 0x03);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    std::vector<uint8_t> voter(pubkey.begin(), pubkey.end());
    // Use an all-zero signature (invalid)
    std::vector<uint8_t> bad_sig(64, 0x00);

    bool ok = vs.CastVote(id, voter, VoteChoice::YES, 50, bad_sig);
    assert(!ok);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTallyRejectBelowQuorum() {
    std::cout << "Test: TallyVotes rejected below quorum" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(1000);  // Require 1000 votes

    Schnorr::PrivateKey privkey{};
    privkey[31] = 0x0B;
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());
    const Schnorr::PublicKey& pubkey = *pubkey_opt;

    std::vector<uint8_t> proposer(32, 0x04);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    // Cast one vote with only 100 power (below quorum)
    std::vector<uint8_t> voter(pubkey.begin(), pubkey.end());
    auto sig = MakeVoteSignature(privkey, id, pubkey, VoteChoice::YES, 100);
    std::vector<uint8_t> sig_bytes(sig.begin(), sig.end());
    vs.CastVote(id, voter, VoteChoice::YES, 100, sig_bytes);

    vs.UpdateBlockHeight(VOTING_START_DELAY + VOTING_PERIOD + 1);
    bool tally_ok = vs.TallyVotes(id);
    assert(tally_ok);

    auto p = vs.GetProposal(id);
    assert(p.has_value() && p->status == ProposalStatus::REJECTED);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestExecuteProposal() {
    std::cout << "Test: ExecuteProposal" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(NO_QUORUM_REQUIRED);

    Schnorr::PrivateKey privkey{};
    privkey[31] = 0x0D;
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());
    const Schnorr::PublicKey& pubkey = *pubkey_opt;

    std::vector<uint8_t> proposer(32, 0x05);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::TREASURY_SPENDING, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    std::vector<uint8_t> voter(pubkey.begin(), pubkey.end());
    auto sig = MakeVoteSignature(privkey, id, pubkey, VoteChoice::YES, 1);
    vs.CastVote(id, voter, VoteChoice::YES, 1, std::vector<uint8_t>(sig.begin(), sig.end()));

    vs.UpdateBlockHeight(VOTING_START_DELAY + VOTING_PERIOD + 1);
    vs.TallyVotes(id);

    // Proposal status is PASSED; execution_time = tally_block + 1000.
    // Cannot execute before execution_time
    bool early_exec = vs.ExecuteProposal(id);
    assert(!early_exec);

    vs.UpdateBlockHeight(VOTING_START_DELAY + VOTING_PERIOD + 1 + 1000);
    bool ok = vs.ExecuteProposal(id);
    assert(ok);

    auto p = vs.GetProposal(id);
    assert(p.has_value() && p->status == ProposalStatus::EXECUTED);

    // Second execution must fail (already EXECUTED)
    bool double_exec = vs.ExecuteProposal(id);
    assert(!double_exec);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetActiveProposals() {
    std::cout << "Test: GetActiveProposals" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(NO_QUORUM_REQUIRED);

    std::vector<uint8_t> proposer(32, 0x06);
    vs.CreateProposal(proposer, ProposalType::GENERAL, "P1", "D1", {});
    vs.CreateProposal(proposer, ProposalType::GENERAL, "P2", "D2", {});

    // Both are PENDING (no votes cast yet), should appear as active
    auto active = vs.GetActiveProposals();
    assert(active.size() == 2);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetProposalVotes() {
    std::cout << "Test: GetProposalVotes" << std::endl;

    VotingSystem vs;
    vs.SetDefaultQuorum(NO_QUORUM_REQUIRED);

    Schnorr::PrivateKey privkey{};
    privkey[31] = 0x0F;
    auto pubkey_opt = Schnorr::GetPublicKey(privkey);
    assert(pubkey_opt.has_value());
    const Schnorr::PublicKey& pubkey = *pubkey_opt;

    std::vector<uint8_t> proposer(32, 0x07);
    uint64_t id = vs.CreateProposal(proposer, ProposalType::GENERAL, "T", "D", {});
    vs.UpdateBlockHeight(VOTING_START_DELAY + 1);

    std::vector<uint8_t> voter(pubkey.begin(), pubkey.end());
    auto sig = MakeVoteSignature(privkey, id, pubkey, VoteChoice::ABSTAIN, 5);
    vs.CastVote(id, voter, VoteChoice::ABSTAIN, 5,
                std::vector<uint8_t>(sig.begin(), sig.end()));

    auto votes = vs.GetProposalVotes(id);
    assert(votes.size() == 1);
    assert(votes[0].choice == VoteChoice::ABSTAIN);
    assert(votes[0].voting_power == 5);

    // Non-existent proposal returns empty
    assert(vs.GetProposalVotes(999).empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVotingParameters() {
    std::cout << "Test: Voting parameter getters/setters" << std::endl;

    VotingSystem vs;
    assert(vs.GetVotingPeriod() == 10000);
    vs.SetVotingPeriod(5000);
    assert(vs.GetVotingPeriod() == 5000);

    vs.SetDefaultQuorum(999);
    assert(vs.GetDefaultQuorum() == 999);

    vs.SetDefaultThreshold(66);
    assert(vs.GetDefaultThreshold() == 66);

    assert(vs.GetBlockHeight() == 0);
    vs.UpdateBlockHeight(42);
    assert(vs.GetBlockHeight() == 42);

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// TreasuryManager Tests
// ---------------------------------------------------------------------------

void TestTreasuryDeposit() {
    std::cout << "Test: TreasuryManager Deposit" << std::endl;

    TreasuryManager tm;
    assert(tm.GetBalance() == 0);

    std::vector<uint8_t> addr(32, 0xCC);
    bool ok = tm.Deposit(500, addr);
    assert(ok);
    assert(tm.GetBalance() == 500);

    // Zero amount must fail
    bool zero = tm.Deposit(0, addr);
    assert(!zero);
    assert(tm.GetBalance() == 500);

    auto txs = tm.GetTransactions();
    assert(txs.size() == 1);
    assert(txs[0].is_deposit);
    assert(txs[0].amount == 500);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTreasuryWithdraw() {
    std::cout << "Test: TreasuryManager Withdraw" << std::endl;

    TreasuryManager tm;
    std::vector<uint8_t> addr(32, 0xDD);
    tm.Deposit(1000, addr);

    // Valid withdrawal linked to proposal
    bool ok = tm.Withdraw(300, addr, /*proposal_id=*/42);
    assert(ok);
    assert(tm.GetBalance() == 700);

    // Withdraw without proposal (proposal_id == 0) must fail
    bool no_prop = tm.Withdraw(100, addr, 0);
    assert(!no_prop);

    // Withdraw more than balance must fail
    bool overflow = tm.Withdraw(800, addr, 43);
    assert(!overflow);
    assert(tm.GetBalance() == 700);

    // Zero amount must fail
    bool zero = tm.Withdraw(0, addr, 44);
    assert(!zero);

    auto txs = tm.GetTransactions();
    assert(txs.size() == 2);  // 1 deposit + 1 withdraw
    assert(!txs[1].is_deposit);
    assert(txs[1].proposal_id == 42);

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// DelegationSystem Tests
// ---------------------------------------------------------------------------

void TestDelegation() {
    std::cout << "Test: DelegationSystem Delegate / GetVotingPower" << std::endl;

    DelegationSystem ds;

    std::vector<uint8_t> alice(32, 0x01);
    std::vector<uint8_t> bob(32, 0x02);

    // Alice delegates 200 to Bob
    bool ok = ds.Delegate(alice, bob, 200);
    assert(ok);
    assert(ds.GetVotingPower(bob) == 200);
    assert(ds.GetVotingPower(alice) == 0);

    // Cannot delegate to self
    bool self_delegation_rejected = ds.Delegate(alice, alice, 10);
    assert(!self_delegation_rejected);

    // Cannot delegate zero
    bool zero = ds.Delegate(alice, bob, 0);
    assert(!zero);

    // Additional delegation accumulates
    ds.Delegate(alice, bob, 100);
    assert(ds.GetVotingPower(bob) == 300);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestUndelegate() {
    std::cout << "Test: DelegationSystem Undelegate" << std::endl;

    DelegationSystem ds;

    std::vector<uint8_t> alice(32, 0x01);
    std::vector<uint8_t> bob(32, 0x02);

    ds.Delegate(alice, bob, 500);
    assert(ds.GetVotingPower(bob) == 500);

    bool ok = ds.Undelegate(alice, bob, 200);
    assert(ok);
    assert(ds.GetVotingPower(bob) == 300);

    // Cannot undelegate more than delegated
    bool over = ds.Undelegate(alice, bob, 400);
    assert(!over);

    // Undelegate the rest; entry is removed
    bool all = ds.Undelegate(alice, bob, 300);
    assert(all);
    assert(ds.GetVotingPower(bob) == 0);

    // Undelegating from non-existent delegator/delegatee must fail
    bool missing = ds.Undelegate(alice, bob, 1);
    assert(!missing);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestDelegationQueries() {
    std::cout << "Test: DelegationSystem GetDelegationsFrom / GetDelegationsTo" << std::endl;

    DelegationSystem ds;

    std::vector<uint8_t> alice(32, 0x01);
    std::vector<uint8_t> bob(32, 0x02);
    std::vector<uint8_t> carol(32, 0x03);

    ds.Delegate(alice, carol, 100);
    ds.Delegate(bob, carol, 50);

    auto from_alice = ds.GetDelegationsFrom(alice);
    assert(from_alice.size() == 1);
    assert(from_alice.at(carol) == 100);

    auto to_carol = ds.GetDelegationsTo(carol);
    assert(to_carol.size() == 2);
    assert(to_carol.at(alice) == 100);
    assert(to_carol.at(bob) == 50);

    // Total voting power for Carol
    assert(ds.GetVotingPower(carol) == 150);

    // Empty results for unknown addresses
    std::vector<uint8_t> unknown(32, 0xFF);
    assert(ds.GetDelegationsFrom(unknown).empty());
    assert(ds.GetDelegationsTo(unknown).empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "ParthenonChain Governance Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        // VotingSystem
        TestCreateProposal();
        TestGetProposalNotFound();
        TestCastVoteAndTally();
        TestCastVoteBeforeVotingStart();
        TestVoteRejectedWithBadSignature();
        TestTallyRejectBelowQuorum();
        TestExecuteProposal();
        TestGetActiveProposals();
        TestGetProposalVotes();
        TestVotingParameters();

        // TreasuryManager
        TestTreasuryDeposit();
        TestTreasuryWithdraw();

        // DelegationSystem
        TestDelegation();
        TestUndelegate();
        TestDelegationQueries();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All governance tests passed! \u2713" << std::endl;
        std::cout << "=============================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
