// ParthenonChain – Treasury Unit Tests

#include "governance/treasury.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

// ---------------------------------------------------------------------------
void TestDepositAndBalance() {
    std::cout << "Test: Deposit and per-track balance" << std::endl;

    Treasury t(2, 0);  // no reserve enforcement

    assert(t.GetTotalBalance() == 0);
    assert(t.Deposit(1000, Addr(0x01), Treasury::Track::CORE_DEVELOPMENT, 1));
    assert(t.Deposit(500,  Addr(0x02), Treasury::Track::GRANTS, 1));
    assert(t.Deposit(200,  Addr(0x03), Treasury::Track::EMERGENCY, 1));

    assert(t.GetTotalBalance()                            == 1700);
    assert(t.GetTrackBalance(Treasury::Track::CORE_DEVELOPMENT) == 1000);
    assert(t.GetTrackBalance(Treasury::Track::GRANTS)           == 500);
    assert(t.GetTrackBalance(Treasury::Track::EMERGENCY)        == 200);
    assert(t.GetReserveBalance()                          == 200);

    // Zero-amount deposit must fail
    assert(!t.Deposit(0, Addr(0x01), Treasury::Track::OPERATIONS, 1));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestSingleTrackSpend() {
    std::cout << "Test: Single-track spending (requires proposal)" << std::endl;

    Treasury t(2, 0);
    t.Deposit(2000, Addr(0x01), Treasury::Track::OPERATIONS, 1);

    // Requires proposal_id != 0
    assert(!t.Spend(100, Addr(0x02), 0, Treasury::Track::OPERATIONS, "bad", 10));

    // Cannot spend from EMERGENCY track via single-sig
    t.Deposit(500, Addr(0x01), Treasury::Track::EMERGENCY, 1);
    assert(!t.Spend(100, Addr(0x02), 1, Treasury::Track::EMERGENCY, "bad", 10));

    // Normal spend
    assert(t.Spend(300, Addr(0x02), 1, Treasury::Track::OPERATIONS, "infra", 10));
    assert(t.GetTrackBalance(Treasury::Track::OPERATIONS) == 1700);

    // Over-balance
    assert(!t.Spend(9999, Addr(0x02), 2, Treasury::Track::OPERATIONS, "big", 10));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestReserveRatio() {
    std::cout << "Test: Reserve ratio enforcement" << std::endl;

    // reserve_ratio_bps = 2000 → emergency must always be >= 20% of total
    Treasury t(2, 2000);
    t.Deposit(800, Addr(0x01), Treasury::Track::OPERATIONS, 1);
    t.Deposit(200, Addr(0x02), Treasury::Track::EMERGENCY, 1);
    // Total = 1000, reserve = 200 (20%) → exactly at limit

    // Spending 100 from OPERATIONS: total becomes 900, emergency still 200 (22%) → OK
    assert(t.Spend(100, Addr(0x03), 1, Treasury::Track::OPERATIONS, "test", 5));

    // Spending another 200 from OPERATIONS: total=700, emergency=200 (28%) → OK
    assert(t.Spend(200, Addr(0x03), 2, Treasury::Track::OPERATIONS, "test", 5));

    // Spending 400 more from OPERATIONS (balance=300): would leave total=300,
    // emergency=200 (67%) → OK actually (emergency percentage higher, reserve is maintained)
    // But spending ALL remaining OPERATIONS (300): total=200, emergency=200 (100%) → OK
    assert(t.Spend(300, Addr(0x03), 3, Treasury::Track::OPERATIONS, "drain ops", 5));
    // Only emergency left
    assert(t.GetTrackBalance(Treasury::Track::OPERATIONS) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestMultiSigSpend() {
    std::cout << "Test: Multi-sig EMERGENCY spending" << std::endl;

    Treasury t(2, 0);  // require 2 signatures
    t.AddGuardian(Addr(0xA1));
    t.AddGuardian(Addr(0xA2));
    t.AddGuardian(Addr(0xA3));
    t.Deposit(5000, Addr(0x01), Treasury::Track::EMERGENCY, 1);

    // Non-guardian cannot propose
    assert(t.ProposeMultiSigSpend(100, Addr(0x09), "test", Addr(0xFF), 10) == 0);

    uint64_t sid = t.ProposeMultiSigSpend(1000, Addr(0x09), "security fix", Addr(0xA1), 10);
    assert(sid > 0);

    // A1 is already initiator (counts as sig), so 1 sig so far
    assert(!t.HasSufficientSignatures(sid));  // need 2

    // A2 signs → now 2 sigs, sufficient
    assert(t.SignMultiSigSpend(sid, Addr(0xA2)));
    assert(t.HasSufficientSignatures(sid));

    // Execute
    assert(t.ExecuteMultiSigSpend(sid, 11));
    assert(t.GetTrackBalance(Treasury::Track::EMERGENCY) == 4000);

    // Cannot execute twice
    assert(!t.ExecuteMultiSigSpend(sid, 12));

    // Non-guardian cannot sign
    assert(!t.SignMultiSigSpend(sid, Addr(0xFF)));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestBudgetPeriod() {
    std::cout << "Test: Budget period spending limits" << std::endl;

    Treasury t(2, 0);
    t.Deposit(5000, Addr(0x01), Treasury::Track::CORE_DEVELOPMENT, 1);

    // Create budget period: blocks 100-200, CORE_DEV limit = 1000
    std::map<Treasury::Track, uint64_t> limits;
    limits[Treasury::Track::CORE_DEVELOPMENT] = 1000;
    uint64_t pid = t.CreateBudgetPeriod(100, 200, limits, /*proposal_id=*/1);
    assert(pid > 0);

    // Before period: no limit applies
    assert(t.IsWithinBudget(Treasury::Track::CORE_DEVELOPMENT, 2000, 50));

    // During period, within limit
    assert(t.IsWithinBudget(Treasury::Track::CORE_DEVELOPMENT, 1000, 150));

    // Spend 700 during period
    assert(t.Spend(700, Addr(0x02), 2, Treasury::Track::CORE_DEVELOPMENT, "dev", 150));

    // Now 300 headroom left
    assert( t.IsWithinBudget(Treasury::Track::CORE_DEVELOPMENT, 300, 150));
    assert(!t.IsWithinBudget(Treasury::Track::CORE_DEVELOPMENT, 301, 150));

    // Cannot create period without proposal
    assert(t.CreateBudgetPeriod(300, 400, limits, 0) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestMilestoneGrant() {
    std::cout << "Test: Milestone grant create / release / revoke" << std::endl;

    Treasury t(2, 0);
    t.Deposit(3000, Addr(0x01), Treasury::Track::GRANTS, 1);

    std::vector<std::pair<std::string, uint64_t>> milestones = {
        {"M1: Design",        500},
        {"M2: Implementation", 1000},
        {"M3: Audit",          500},
    };

    uint64_t gid = t.CreateGrant(/*proposal_id=*/1, Addr(0xBB), "protocol v2",
                                 milestones, 10);
    assert(gid > 0);

    // Balance reduced by total (2000)
    assert(t.GetTrackBalance(Treasury::Track::GRANTS) == 1000);

    // Release M1 (index 0)
    assert(t.ReleaseMilestone(gid, 0, 2, 20));
    auto grant = t.GetGrant(gid);
    assert(grant.has_value() && grant->released_amount == 500);

    // Cannot release already-released milestone
    assert(!t.ReleaseMilestone(gid, 0, 3, 21));

    // Revoke remaining milestones → M2 and M3 refunded (1500)
    assert(t.RevokeGrant(gid, 4, 30));
    assert(t.GetTrackBalance(Treasury::Track::GRANTS) == 1000 + 1500);

    // Cannot revoke twice
    assert(!t.RevokeGrant(gid, 5, 31));

    // Create grant with more than balance must fail
    assert(t.CreateGrant(1, Addr(0xCC), "big", {{"all", 99999}}, 35) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTransactionLog() {
    std::cout << "Test: Audit transaction log" << std::endl;

    Treasury t(1, 0);
    t.AddGuardian(Addr(0xAA));

    t.Deposit(1000, Addr(0x01), Treasury::Track::OPERATIONS, 1);
    t.Spend(200, Addr(0x02), 1, Treasury::Track::OPERATIONS, "servers", 5);

    auto all_txs = t.GetTransactions();
    assert(all_txs.size() == 2);
    assert(all_txs[0].is_deposit && all_txs[0].amount == 1000);
    assert(!all_txs[1].is_deposit && all_txs[1].amount == 200);

    auto ops_txs = t.GetTransactionsByTrack(Treasury::Track::OPERATIONS);
    assert(ops_txs.size() == 2);

    auto em_txs = t.GetTransactionsByTrack(Treasury::Track::EMERGENCY);
    assert(em_txs.empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGuardianManagement() {
    std::cout << "Test: Guardian add / remove / duplicate" << std::endl;

    Treasury t(2, 0);

    assert(t.AddGuardian(Addr(0x01)));
    assert(t.IsGuardian(Addr(0x01)));
    assert(!t.AddGuardian(Addr(0x01)));  // duplicate
    assert(!t.AddGuardian({}));           // empty addr

    assert(t.RemoveGuardian(Addr(0x01)));
    assert(!t.IsGuardian(Addr(0x01)));
    assert(!t.RemoveGuardian(Addr(0x01)));  // already removed

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "Treasury Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestDepositAndBalance();
        TestSingleTrackSpend();
        TestReserveRatio();
        TestMultiSigSpend();
        TestBudgetPeriod();
        TestMilestoneGrant();
        TestTransactionLog();
        TestGuardianManagement();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All Treasury tests passed! \u2713" << std::endl;
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
