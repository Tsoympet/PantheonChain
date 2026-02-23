// ParthenonChain – StakingRegistry Unit Tests

#include "governance/staking.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

void TestStakeAndVotingPower() {
    std::cout << "Test: Stake and GetVotingPower" << std::endl;

    StakingRegistry sr(100);

    assert(sr.GetStake(Addr(0x01))        == 0);
    assert(sr.GetVotingPower(Addr(0x01))  == 0);
    assert(sr.GetTotalStaked()            == 0);

    assert(sr.Stake(Addr(0x01), 1000, 0, 10));
    assert(sr.GetStake(Addr(0x01))        == 1000);
    assert(sr.GetVotingPower(Addr(0x01))  == 1000);

    // Staking more accumulates
    assert(sr.Stake(Addr(0x01), 500, 0, 11));
    assert(sr.GetStake(Addr(0x01))        == 1500);

    assert(sr.Stake(Addr(0x02), 200, 0, 12));
    assert(sr.GetTotalStaked()            == 1700);
    assert(sr.GetTotalVotingPower()       == 1700);

    // Edge cases
    assert(!sr.Stake({}, 100, 0, 1));   // empty address
    assert(!sr.Stake(Addr(0x03), 0, 0, 1));  // zero amount

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestRequestAndClaimUnstake() {
    std::cout << "Test: RequestUnstake and ClaimUnstake with cooldown" << std::endl;

    StakingRegistry sr(/*cooldown=*/200);
    sr.Stake(Addr(0x01), 1000, 0, 0);

    // Request 400 unstake
    assert(sr.RequestUnstake(Addr(0x01), 400, 100));

    // Voting power reduced immediately by pending amount
    assert(sr.GetVotingPower(Addr(0x01))  == 600);
    assert(sr.GetStake(Addr(0x01))        == 1000);  // still staked

    // Cannot request again (one pending per address)
    assert(!sr.RequestUnstake(Addr(0x01), 100, 101));

    // Claim before cooldown expires must fail
    assert(!sr.ClaimUnstake(Addr(0x01), 299));

    // Claim after cooldown
    assert(sr.ClaimUnstake(Addr(0x01), 300));  // claimable at 100+200=300
    assert(sr.GetStake(Addr(0x01))        == 600);
    assert(sr.GetVotingPower(Addr(0x01))  == 600);

    // Cannot claim twice
    assert(!sr.ClaimUnstake(Addr(0x01), 400));

    // Over-amount request must fail
    assert(!sr.RequestUnstake(Addr(0x01), 601, 400));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestStakeLock() {
    std::cout << "Test: Stake lock prevents early unstake" << std::endl;

    StakingRegistry sr(50);
    sr.Stake(Addr(0x01), 1000, /*lock_period=*/500, /*block=*/100);

    // Locked until block 600
    assert(sr.IsStakeLocked(Addr(0x01), 599));
    assert(!sr.IsStakeLocked(Addr(0x01), 600));

    // Cannot unstake while locked
    assert(!sr.RequestUnstake(Addr(0x01), 100, 150));

    // Can unstake after lock expires
    assert(sr.RequestUnstake(Addr(0x01), 100, 600));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestSlash() {
    std::cout << "Test: Slash" << std::endl;

    StakingRegistry sr(100);
    sr.Stake(Addr(0x01), 1000, 0, 0);
    sr.RequestUnstake(Addr(0x01), 400, 0);  // 400 pending

    // Slash 300
    assert(sr.Slash(Addr(0x01), 300, "governance attack", 50));
    assert(sr.GetStake(Addr(0x01)) == 700);

    // Slash history recorded
    assert(sr.GetSlashHistory().size() == 1);
    assert(sr.GetSlashHistory()[0].reason == "governance attack");
    assert(sr.GetSlashHistory()[0].amount == 300);

    // Cannot slash non-existent address
    assert(!sr.Slash(Addr(0xFF), 100, "bad", 50));

    // Cannot slash more than staked
    assert(!sr.Slash(Addr(0x01), 9999, "too much", 50));

    // Zero-amount slash must fail
    assert(!sr.Slash(Addr(0x01), 0, "zero", 50));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestStakeRecordQuery() {
    std::cout << "Test: GetStakeRecord and GetUnstakeRequest" << std::endl;

    StakingRegistry sr(100);
    assert(!sr.GetStakeRecord(Addr(0x01)).has_value());
    assert(!sr.GetUnstakeRequest(Addr(0x01)).has_value());

    sr.Stake(Addr(0x01), 500, 0, 10);
    auto rec = sr.GetStakeRecord(Addr(0x01));
    assert(rec.has_value() && rec->staked_amount == 500);

    sr.RequestUnstake(Addr(0x01), 200, 20);
    auto req = sr.GetUnstakeRequest(Addr(0x01));
    assert(req.has_value() && req->amount == 200);
    assert(req->claimable_at_block == 120);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTotalVotingPower() {
    std::cout << "Test: GetTotalVotingPower excludes pending unstake" << std::endl;

    StakingRegistry sr(50);
    sr.Stake(Addr(0x01), 1000, 0, 0);
    sr.Stake(Addr(0x02), 500, 0, 0);

    assert(sr.GetTotalVotingPower() == 1500);

    sr.RequestUnstake(Addr(0x01), 300, 0);
    assert(sr.GetTotalVotingPower() == 1200);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "StakingRegistry Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestStakeAndVotingPower();
        TestRequestAndClaimUnstake();
        TestStakeLock();
        TestSlash();
        TestStakeRecordQuery();
        TestTotalVotingPower();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All StakingRegistry tests passed! \u2713" << std::endl;
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
