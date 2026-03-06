// ParthenonChain – One-Address-One-Vote Unit Tests
//
// Staking is DISABLED.  Governance uses one-address-one-vote (1A1V):
// every token holder gets exactly 1 vote regardless of balance size.

#include "governance/balance_voting.h"
#include "governance/staking.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

// ---------------------------------------------------------------------------
// StakingRegistry: staking operations are disabled
// ---------------------------------------------------------------------------
void TestStakingDisabled() {
    std::cout << "Test: Staking operations are disabled" << std::endl;

    StakingRegistry sr(100);

    // All mutation methods must return false
    assert(!sr.Stake(Addr(0x01), 1000, 0, 10));
    assert(!sr.RequestUnstake(Addr(0x01), 100, 20));
    assert(!sr.ClaimUnstake(Addr(0x01), 300));

    // No stakes were created
    assert(sr.GetStake(Addr(0x01))       == 0);
    assert(sr.GetVotingPower(Addr(0x01)) == 0);
    assert(sr.GetTotalStaked()           == 0);
    assert(sr.GetTotalVotingPower()      == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

// ---------------------------------------------------------------------------
// BalanceVotingRegistry: one-address-one-vote semantics
// ---------------------------------------------------------------------------
void TestOneAddressOneVote_Basic() {
    std::cout << "Test: 1A1V – every holder has exactly 1 vote" << std::endl;

    BalanceVotingRegistry bvr;

    // Address with no balance → 0 votes
    assert(bvr.GetVotingPower(Addr(0x01)) == 0);
    assert(bvr.GetTotalVotingPower()      == 0);

    // Address with any positive balance → exactly 1 vote
    bvr.UpdateBalance(Addr(0x01), 1);        // minimum holding
    assert(bvr.GetVotingPower(Addr(0x01)) == 1);
    assert(bvr.GetTotalVotingPower()      == 1);

    bvr.UpdateBalance(Addr(0x02), 1000000);  // whale-level holding
    assert(bvr.GetVotingPower(Addr(0x02)) == 1);
    assert(bvr.GetTotalVotingPower()      == 2);

    // Large balance increase does NOT increase vote power
    bvr.UpdateBalance(Addr(0x01), 999999999999ULL);
    assert(bvr.GetVotingPower(Addr(0x01)) == 1);   // still exactly 1
    assert(bvr.GetTotalVotingPower()      == 2);    // still 2 voters

    // Removing balance removes the vote
    bvr.UpdateBalance(Addr(0x01), 0);
    assert(bvr.GetVotingPower(Addr(0x01)) == 0);
    assert(bvr.GetTotalVotingPower()      == 1);
    assert(bvr.Size()                     == 1);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestOneAddressOneVote_SetBalances() {
    std::cout << "Test: 1A1V – SetBalances replaces map, each holder = 1 vote" << std::endl;

    BalanceVotingRegistry bvr;
    bvr.UpdateBalance(Addr(0x01), 9999);   // will be erased

    std::map<std::vector<uint8_t>, uint64_t> snap;
    snap[Addr(0x10)] = 300;
    snap[Addr(0x11)] = 700;
    snap[Addr(0x12)] = 0;   // zero entries should be skipped

    bvr.SetBalances(snap);

    assert(bvr.GetVotingPower(Addr(0x01)) == 0);   // old entry gone
    assert(bvr.GetVotingPower(Addr(0x10)) == 1);   // has balance → 1 vote
    assert(bvr.GetVotingPower(Addr(0x11)) == 1);   // has balance → 1 vote
    assert(bvr.GetVotingPower(Addr(0x12)) == 0);   // zero not stored
    assert(bvr.GetTotalVotingPower()      == 2);   // 2 eligible voters
    assert(bvr.Size()                     == 2);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestOneAddressOneVote_GetAllPowers() {
    std::cout << "Test: 1A1V – GetAllVotingPowers returns (addr, 1) for each holder" << std::endl;

    BalanceVotingRegistry bvr;
    bvr.UpdateBalance(Addr(0xAA), 400);
    bvr.UpdateBalance(Addr(0xBB), 600);
    bvr.UpdateBalance(Addr(0xCC), 1);    // minimum holding still counts

    auto powers = bvr.GetAllVotingPowers();
    assert(powers.size() == 3);

    // Every entry must have voting_power == 1
    for (const auto& [addr, power] : powers) {
        assert(power == 1);
    }

    // Sum of all powers == number of holders
    uint64_t total = 0;
    for (const auto& [addr, power] : powers) {
        total += power;
    }
    assert(total == 3);
    assert(bvr.GetTotalVotingPower() == 3);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestOneAddressOneVote_WhaleParity() {
    std::cout << "Test: 1A1V – whale and small holder have identical vote weight" << std::endl;

    BalanceVotingRegistry bvr;
    bvr.UpdateBalance(Addr(0x01), 1u);              // smallest possible holding
    bvr.UpdateBalance(Addr(0x02), UINT64_MAX);       // theoretical maximum

    assert(bvr.GetVotingPower(Addr(0x01)) == bvr.GetVotingPower(Addr(0x02)));
    assert(bvr.GetVotingPower(Addr(0x01)) == 1);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "One-Address-One-Vote (1A1V) Unit Tests" << std::endl;
    std::cout << "  (Staking disabled; every holder = 1 vote)" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    try {
        TestStakingDisabled();
        TestOneAddressOneVote_Basic();
        TestOneAddressOneVote_SetBalances();
        TestOneAddressOneVote_GetAllPowers();
        TestOneAddressOneVote_WhaleParity();

        std::cout << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "All 1A1V tests passed! \u2713" << std::endl;
        std::cout << "==========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
