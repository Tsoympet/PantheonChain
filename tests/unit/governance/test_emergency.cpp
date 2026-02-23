// ParthenonChain – EmergencyCouncil Unit Tests

#include "governance/emergency.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

void TestGuardianManagement() {
    std::cout << "Test: Guardian add / remove / duplicate" << std::endl;

    EmergencyCouncil ec(2, 1000);

    assert(ec.AddGuardian(Addr(0x01), "security", 0));
    assert(ec.IsGuardian(Addr(0x01)));
    assert(!ec.AddGuardian(Addr(0x01), "dup", 0));  // duplicate
    assert(!ec.AddGuardian({}, "empty", 0));          // empty addr

    assert(ec.AddGuardian(Addr(0x02), "core-dev", 0));
    assert(ec.GetGuardians().size() == 2);

    assert(ec.RemoveGuardian(Addr(0x01)));
    assert(!ec.IsGuardian(Addr(0x01)));
    assert(!ec.RemoveGuardian(Addr(0x01)));  // already removed

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestProposeAndSign() {
    std::cout << "Test: ProposeAction and SignAction" << std::endl;

    EmergencyCouncil ec(2, 1000);
    ec.AddGuardian(Addr(0xA1), "g1", 0);
    ec.AddGuardian(Addr(0xA2), "g2", 0);
    ec.AddGuardian(Addr(0xA3), "g3", 0);

    // Non-guardian cannot propose
    assert(ec.ProposeAction(EmergencyCouncil::ActionType::CANCEL_PROPOSAL,
                             "malicious prop", Addr(0xFF), 42, 0) == 0);

    uint64_t aid = ec.ProposeAction(EmergencyCouncil::ActionType::CANCEL_PROPOSAL,
                                    "cancel malicious proposal #42",
                                    Addr(0xA1), 42, 100);
    assert(aid == 1);

    auto action = ec.GetAction(aid);
    assert(action.has_value());
    assert(action->signers.size() == 1);  // initiator counted
    assert(!ec.HasSufficientSignatures(aid));  // need 2

    // Non-guardian cannot sign
    assert(!ec.SignAction(aid, Addr(0xFF), 101));

    // A2 signs → 2 sigs
    assert(ec.SignAction(aid, Addr(0xA2), 101));
    assert(ec.HasSufficientSignatures(aid));

    // Duplicate sign has no effect (set insert is idempotent)
    ec.SignAction(aid, Addr(0xA2), 102);
    assert(ec.GetAction(aid)->signers.size() == 2);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestExecuteAction() {
    std::cout << "Test: ExecuteAction and idempotency" << std::endl;

    EmergencyCouncil ec(2, 1000);
    ec.AddGuardian(Addr(0xB1), "g1", 0);
    ec.AddGuardian(Addr(0xB2), "g2", 0);

    uint64_t aid = ec.ProposeAction(EmergencyCouncil::ActionType::CUSTOM,
                                    "custom emergency", Addr(0xB1), 0, 0);
    ec.SignAction(aid, Addr(0xB2), 1);

    // Execute succeeds
    assert(ec.ExecuteAction(aid, 5));

    auto act = ec.GetAction(aid);
    assert(act.has_value() && act->executed);
    assert(act->executed_at_block == 5);

    // Cannot execute twice
    assert(!ec.ExecuteAction(aid, 6));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestActionExpiry() {
    std::cout << "Test: Action expires after TTL" << std::endl;

    EmergencyCouncil ec(/*required_sigs=*/1, /*ttl=*/100);
    ec.AddGuardian(Addr(0xC1), "g1", 0);

    uint64_t aid = ec.ProposeAction(EmergencyCouncil::ActionType::CUSTOM,
                                    "test", Addr(0xC1), 0, 50);
    // Expires at block 50+100=150

    assert(!ec.IsExpired(aid, 149));
    assert( ec.IsExpired(aid, 151));

    // Cannot sign after expiry
    ec.AddGuardian(Addr(0xC2), "g2", 0);
    assert(!ec.SignAction(aid, Addr(0xC2), 200));

    // Cannot execute after expiry
    assert(!ec.ExecuteAction(aid, 200));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestPauseGovernance() {
    std::cout << "Test: PAUSE_GOVERNANCE action sets paused flag" << std::endl;

    EmergencyCouncil ec(2, 5000);
    ec.AddGuardian(Addr(0xD1), "g1", 0);
    ec.AddGuardian(Addr(0xD2), "g2", 0);

    assert(!ec.IsGovernancePaused());

    uint64_t aid = ec.ProposeAction(EmergencyCouncil::ActionType::PAUSE_GOVERNANCE,
                                    "pause for security audit", Addr(0xD1), 0, 0);
    ec.SignAction(aid, Addr(0xD2), 1);
    ec.ExecuteAction(aid, 2);

    assert(ec.IsGovernancePaused());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestInsufficientSigsBlocksExecution() {
    std::cout << "Test: Insufficient signatures block execution" << std::endl;

    EmergencyCouncil ec(3, 1000);  // need 3
    ec.AddGuardian(Addr(0xE1), "g1", 0);
    ec.AddGuardian(Addr(0xE2), "g2", 0);
    ec.AddGuardian(Addr(0xE3), "g3", 0);

    uint64_t aid = ec.ProposeAction(EmergencyCouncil::ActionType::CUSTOM,
                                    "needs 3 sigs", Addr(0xE1), 0, 0);
    ec.SignAction(aid, Addr(0xE2), 1);
    // Only 2 sigs, need 3
    assert(!ec.ExecuteAction(aid, 2));

    ec.SignAction(aid, Addr(0xE3), 2);
    assert(ec.ExecuteAction(aid, 3));

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "EmergencyCouncil Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestGuardianManagement();
        TestProposeAndSign();
        TestExecuteAction();
        TestActionExpiry();
        TestPauseGovernance();
        TestInsufficientSigsBlocksExecution();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All EmergencyCouncil tests passed! \u2713" << std::endl;
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
