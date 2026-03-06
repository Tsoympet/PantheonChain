// ParthenonChain – Governance Staking / Balance-Voting Unit Tests
//
// Staking is DISABLED; Stake() and RequestUnstake() always return false.
// Voting power is now derived from token balances via BalanceVotingRegistry.

#include "governance/balance_voting.h"
#include "governance/staking.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

// ---------------------------------------------------------------------------
// StakingRegistry: verify that staking operations are now disabled
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
// BalanceVotingRegistry: core voting-power queries
// ---------------------------------------------------------------------------
void TestBalanceVotingBasic() {
    std::cout << "Test: BalanceVotingRegistry – basic voting power" << std::endl;

    BalanceVotingRegistry bvr;

    assert(bvr.GetVotingPower(Addr(0x01)) == 0);
    assert(bvr.GetTotalVotingPower()      == 0);

    bvr.UpdateBalance(Addr(0x01), 1000);
    assert(bvr.GetVotingPower(Addr(0x01)) == 1000);
    assert(bvr.GetTotalVotingPower()      == 1000);

    bvr.UpdateBalance(Addr(0x02), 500);
    assert(bvr.GetVotingPower(Addr(0x02)) == 500);
    assert(bvr.GetTotalVotingPower()      == 1500);

    // Update existing entry
    bvr.UpdateBalance(Addr(0x01), 2000);
    assert(bvr.GetVotingPower(Addr(0x01)) == 2000);
    assert(bvr.GetTotalVotingPower()      == 2500);

    // Zero removes entry
    bvr.UpdateBalance(Addr(0x01), 0);
    assert(bvr.GetVotingPower(Addr(0x01)) == 0);
    assert(bvr.GetTotalVotingPower()      == 500);
    assert(bvr.Size()                     == 1);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestBalanceVotingSetBalances() {
    std::cout << "Test: BalanceVotingRegistry – SetBalances replaces map" << std::endl;

    BalanceVotingRegistry bvr;
    bvr.UpdateBalance(Addr(0x01), 9999);

    std::map<std::vector<uint8_t>, uint64_t> snap;
    snap[Addr(0x10)] = 300;
    snap[Addr(0x11)] = 700;
    snap[Addr(0x12)] = 0;  // zero entries should be skipped

    bvr.SetBalances(snap);

    assert(bvr.GetVotingPower(Addr(0x01)) == 0);  // old entry gone
    assert(bvr.GetVotingPower(Addr(0x10)) == 300);
    assert(bvr.GetVotingPower(Addr(0x11)) == 700);
    assert(bvr.GetVotingPower(Addr(0x12)) == 0);  // zero not stored
    assert(bvr.GetTotalVotingPower()      == 1000);
    assert(bvr.Size()                     == 2);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestBalanceVotingGetAllPowers() {
    std::cout << "Test: BalanceVotingRegistry – GetAllVotingPowers" << std::endl;

    BalanceVotingRegistry bvr;
    bvr.UpdateBalance(Addr(0xAA), 400);
    bvr.UpdateBalance(Addr(0xBB), 600);

    auto powers = bvr.GetAllVotingPowers();
    assert(powers.size() == 2);

    uint64_t total = 0;
    for (const auto& [addr, power] : powers) {
        total += power;
    }
    assert(total == 1000);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "Governance Voting Unit Tests" << std::endl;
    std::cout << "  (Staking disabled; balance-based voting)" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    try {
        TestStakingDisabled();
        TestBalanceVotingBasic();
        TestBalanceVotingSetBalances();
        TestBalanceVotingGetAllPowers();

        std::cout << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "All governance voting tests passed! \u2713" << std::endl;
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
