// ParthenonChain – VestingRegistry Unit Tests

#include "governance/vesting.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

void TestCreateSchedule() {
    std::cout << "Test: CreateSchedule basic validation" << std::endl;

    VestingRegistry reg;

    // Valid schedule
    uint64_t sid = reg.CreateSchedule(Addr(0x01), 12000, 0, 1000, 11000);
    assert(sid > 0);
    assert(reg.Count() == 1);

    // Empty beneficiary → reject
    assert(reg.CreateSchedule({}, 1000, 0, 0, 100) == 0);

    // Zero amount → reject
    assert(reg.CreateSchedule(Addr(0x02), 0, 0, 0, 100) == 0);

    // Zero duration → reject
    assert(reg.CreateSchedule(Addr(0x02), 1000, 0, 0, 0) == 0);

    auto s = reg.GetSchedule(sid);
    assert(s.has_value());
    assert(s->total_amount   == 12000);
    assert(s->cliff_blocks   == 1000);
    assert(s->duration_blocks == 11000);
    assert(!s->revoked);
    assert(s->claimed_amount == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestLinearVestingAfterCliff() {
    std::cout << "Test: Linear vesting (no cliff)" << std::endl;

    VestingRegistry reg;
    // start=0, cliff=0, duration=10000, total=10000
    uint64_t sid = reg.CreateSchedule(Addr(0x01), 10000, 0, 0, 10000);

    // At block 0: nothing vested (elapsed=0)
    assert(reg.GetTotalVested(sid, 0) == 0);
    assert(reg.GetClaimable(sid, 0)   == 0);

    // At block 5000: 50 % vested
    assert(reg.GetTotalVested(sid, 5000) == 5000);
    assert(reg.GetClaimable(sid, 5000)   == 5000);

    // At block 10000: fully vested
    assert(reg.GetTotalVested(sid, 10000) == 10000);

    // Beyond duration: still total (cap at 100 %)
    assert(reg.GetTotalVested(sid, 99999) == 10000);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestCliffBlocking() {
    std::cout << "Test: Cliff blocks release until cliff expires" << std::endl;

    VestingRegistry reg;
    // start=100, cliff=500, duration=1000, total=1000
    uint64_t sid = reg.CreateSchedule(Addr(0x01), 1000, 100, 500, 1000);

    // Before cliff: nothing claimable
    assert(reg.GetTotalVested(sid, 0)   == 0);
    assert(reg.GetTotalVested(sid, 599) == 0);  // cliff_end = 100+500 = 600

    // At cliff_end (block 600): starts vesting, elapsed=0 → still 0
    assert(reg.GetTotalVested(sid, 600) == 0);

    // At block 700 (elapsed=100 out of 1000): 10 % = 100
    assert(reg.GetTotalVested(sid, 700) == 100);

    // At block 1600 (elapsed=1000 = duration): 100 % = 1000
    assert(reg.GetTotalVested(sid, 1600) == 1000);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestClaimReducesClaimable() {
    std::cout << "Test: Claim() advances claimed_amount; GetClaimable reflects remainder" << std::endl;

    VestingRegistry reg;
    // start=0, cliff=0, duration=1000, total=1000
    uint64_t sid = reg.CreateSchedule(Addr(0x01), 1000, 0, 0, 1000);

    // At block 500: 500 claimable
    assert(reg.GetClaimable(sid, 500) == 500);

    uint64_t claimed = reg.Claim(sid, 500);
    assert(claimed == 500);
    assert(reg.GetClaimable(sid, 500) == 0);   // already claimed
    assert(reg.GetClaimable(sid, 750) == 250); // 750 - 500 more vested by block 750

    // Claim again at 750
    claimed = reg.Claim(sid, 750);
    assert(claimed == 250);
    assert(reg.GetClaimable(sid, 750) == 0);

    // At block 1000 (full vest): 250 more claimable
    assert(reg.GetClaimable(sid, 1000) == 250);
    claimed = reg.Claim(sid, 1000);
    assert(claimed == 250);
    assert(reg.GetClaimable(sid, 9999) == 0);  // all claimed

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestRevoke() {
    std::cout << "Test: Revoke returns unvested amount and blocks further claims" << std::endl;

    VestingRegistry reg;
    // start=0, cliff=0, duration=1000, total=1000
    uint64_t sid = reg.CreateSchedule(Addr(0x01), 1000, 0, 0, 1000);

    // Claim 300 at block 300
    reg.Claim(sid, 300);

    // Revoke at block 400 (400 vested, 300 already claimed → 100 not yet claimed)
    // Reclaimable = 1000 - 400 = 600 (unvested tokens back to treasury)
    uint64_t reclaimable = reg.Revoke(sid, /*proposal_id=*/1, 400);
    assert(reclaimable == 600);

    auto s = reg.GetSchedule(sid);
    assert(s.has_value() && s->revoked);
    assert(s->revoked_at_block == 400);

    // Cannot claim after revocation
    assert(reg.GetClaimable(sid, 500) == 0);
    assert(reg.Claim(sid, 500) == 0);

    // Cannot revoke twice
    assert(reg.Revoke(sid, 2, 500) == 0);

    // Requires proposal_id
    uint64_t sid2 = reg.CreateSchedule(Addr(0x02), 500, 0, 0, 1000);
    assert(reg.Revoke(sid2, 0, 100) == 0);  // missing proposal

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetReclaimable() {
    std::cout << "Test: GetReclaimable = total - vested" << std::endl;

    VestingRegistry reg;
    uint64_t sid = reg.CreateSchedule(Addr(0x01), 1000, 0, 0, 1000);

    // Before vesting starts: all reclaimable
    assert(reg.GetReclaimable(sid, 0) == 1000);

    // At 50 %: 500 reclaimable
    assert(reg.GetReclaimable(sid, 500) == 500);

    // Fully vested: 0 reclaimable
    assert(reg.GetReclaimable(sid, 1000) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetSchedulesForBeneficiary() {
    std::cout << "Test: GetSchedulesForBeneficiary" << std::endl;

    VestingRegistry reg;
    reg.CreateSchedule(Addr(0x01), 1000, 0, 0, 100);
    reg.CreateSchedule(Addr(0x01), 2000, 0, 0, 200);
    reg.CreateSchedule(Addr(0x02), 3000, 0, 0, 300);

    auto schedules01 = reg.GetSchedulesForBeneficiary(Addr(0x01));
    assert(schedules01.size() == 2);

    auto schedules02 = reg.GetSchedulesForBeneficiary(Addr(0x02));
    assert(schedules02.size() == 1);

    auto schedules03 = reg.GetSchedulesForBeneficiary(Addr(0x03));
    assert(schedules03.empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "VestingRegistry Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestCreateSchedule();
        TestLinearVestingAfterCliff();
        TestCliffBlocking();
        TestClaimReducesClaimable();
        TestRevoke();
        TestGetReclaimable();
        TestGetSchedulesForBeneficiary();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All VestingRegistry tests passed! \u2713" << std::endl;
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
