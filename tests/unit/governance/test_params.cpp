// ParthenonChain – GovernanceParams Unit Tests

#include "governance/params.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

void TestDefaults() {
    std::cout << "Test: Default parameters" << std::endl;

    GovernanceParams gp;
    const auto& p = gp.Get();

    assert(p.voting_period_blocks         == 10000);
    assert(p.voting_delay_blocks          == 100);
    assert(p.execution_delay_blocks       == 1000);
    assert(p.default_threshold_bps        == 5000);
    assert(p.constitutional_threshold_bps == 6667);
    assert(p.quadratic_voting_enabled);
    assert(p.boule_size                   == 21);
    assert(p.boule_screening_required);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestUpdateRequiresProposalId() {
    std::cout << "Test: UpdateParam requires non-zero proposal_id" << std::endl;

    GovernanceParams gp;
    // proposal_id == 0 must be rejected
    assert(!gp.UpdateParam("voting_period_blocks", 5000, 0, 100));
    assert(gp.Get().voting_period_blocks == 10000);  // unchanged

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestUpdateValidParam() {
    std::cout << "Test: Update valid parameter with proposal" << std::endl;

    GovernanceParams gp;
    assert(gp.UpdateParam("voting_period_blocks", 5000, /*proposal_id=*/1, 200));
    assert(gp.Get().voting_period_blocks == 5000);

    // Change history recorded
    assert(gp.GetChangeHistory().size() == 1);
    const auto& ch = gp.GetChangeHistory()[0];
    assert(ch.key             == "voting_period_blocks");
    assert(ch.old_value       == 10000);
    assert(ch.new_value       == 5000);
    assert(ch.proposal_id     == 1);
    assert(ch.changed_at_block == 200);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestConstitutionalFloors() {
    std::cout << "Test: Constitutional floors are enforced" << std::endl;

    GovernanceParams gp;

    // voting_period_blocks: minimum = 100
    assert(!gp.UpdateParam("voting_period_blocks", 99, 1, 0));
    assert(!gp.UpdateParam("voting_period_blocks", 0,  1, 0));
    assert( gp.UpdateParam("voting_period_blocks", 100, 1, 0));

    // voting_period_blocks: maximum = 504000
    assert(!gp.UpdateParam("voting_period_blocks", 504001, 2, 0));
    assert( gp.UpdateParam("voting_period_blocks", 504000, 2, 0));

    // constitutional_threshold_bps: must be > 50 % (i.e. >= 5001)
    assert(!gp.UpdateParam("constitutional_threshold_bps", 5000, 3, 0));
    assert( gp.UpdateParam("constitutional_threshold_bps", 5001, 3, 0));

    // default_threshold_bps: must be >= 3334 (> 1/3)
    assert(!gp.UpdateParam("default_threshold_bps", 3333, 4, 0));
    assert( gp.UpdateParam("default_threshold_bps", 3334, 4, 0));

    // boule_size: 1..500
    assert(!gp.UpdateParam("boule_size", 0,   5, 0));
    assert(!gp.UpdateParam("boule_size", 501, 5, 0));
    assert( gp.UpdateParam("boule_size", 7,   5, 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestUpdateBoolParam() {
    std::cout << "Test: UpdateBoolParam" << std::endl;

    GovernanceParams gp;
    assert(gp.Get().quadratic_voting_enabled);

    assert(!gp.UpdateBoolParam("quadratic_voting_enabled", false, 0, 0));  // no proposal
    assert( gp.UpdateBoolParam("quadratic_voting_enabled", false, 1, 0));
    assert(!gp.Get().quadratic_voting_enabled);

    assert( gp.UpdateBoolParam("boule_screening_required", false, 2, 10));
    assert(!gp.Get().boule_screening_required);

    // Unknown key
    assert(!gp.UpdateBoolParam("nonexistent_key", true, 3, 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestUnknownKey() {
    std::cout << "Test: Unknown parameter key is rejected" << std::endl;

    GovernanceParams gp;
    assert(!gp.UpdateParam("unknown_param", 42, 1, 0));
    assert(gp.GetChangeHistory().empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestMultipleParamUpdates() {
    std::cout << "Test: Multiple parameter updates accumulate in history" << std::endl;

    GovernanceParams gp;
    gp.UpdateParam("default_quorum",       500000, 1, 100);
    gp.UpdateParam("voting_period_blocks", 20000,  2, 200);
    gp.UpdateParam("execution_delay_blocks", 2000, 3, 300);

    assert(gp.Get().default_quorum         == 500000);
    assert(gp.Get().voting_period_blocks   == 20000);
    assert(gp.Get().execution_delay_blocks == 2000);
    assert(gp.GetChangeHistory().size()    == 3);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "GovernanceParams Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestDefaults();
        TestUpdateRequiresProposalId();
        TestUpdateValidParam();
        TestConstitutionalFloors();
        TestUpdateBoolParam();
        TestUnknownKey();
        TestMultipleParamUpdates();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All GovernanceParams tests passed! \u2713" << std::endl;
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
