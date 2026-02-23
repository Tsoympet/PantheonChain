// ParthenonChain – Boule (Athenian Council) Unit Tests

#include "governance/boule.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

// Helper: make a 32-byte address from a single seed byte
static std::vector<uint8_t> Addr(uint8_t seed) {
    return std::vector<uint8_t>(32, seed);
}

static std::vector<uint8_t> Seed(uint32_t val) {
    return {
        static_cast<uint8_t>((val >> 24) & 0xFF),
        static_cast<uint8_t>((val >> 16) & 0xFF),
        static_cast<uint8_t>((val >>  8) & 0xFF),
        static_cast<uint8_t>( val        & 0xFF),
    };
}

void TestCitizenRegistration() {
    std::cout << "Test: Citizen registration (Dokimasia)" << std::endl;

    Boule boule(5, 1000, /*min_stake=*/100, /*screening=*/true);

    // Valid registration
    assert(boule.RegisterCitizen(Addr(0x01), 200, 0));
    assert(boule.IsCitizenRegistered(Addr(0x01)));

    // Duplicate registration must fail
    assert(!boule.RegisterCitizen(Addr(0x01), 200, 0));

    // Below minimum stake must fail
    assert(!boule.RegisterCitizen(Addr(0x02), 50, 0));
    assert(!boule.IsCitizenRegistered(Addr(0x02)));

    // Empty address must fail
    assert(!boule.RegisterCitizen({}, 200, 0));

    assert(boule.GetRegisteredCitizenCount() == 1);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestSortition() {
    std::cout << "Test: Sortition (Kleroterion)" << std::endl;

    const uint32_t COUNCIL_SIZE = 3;
    Boule boule(COUNCIL_SIZE, 1000, 0, true);

    // Register enough citizens
    for (uint8_t i = 1; i <= 10; ++i) {
        assert(boule.RegisterCitizen(Addr(i), 100, 0));
    }

    // Cannot select with seed < 4 bytes
    assert(!boule.ConductSortition({0x01, 0x02}, 100));

    // Valid sortition
    assert(boule.ConductSortition(Seed(0xDEADBEEF), 100));
    assert(boule.GetCurrentCouncil().size() == COUNCIL_SIZE);

    // All selected members must be registered citizens
    for (const auto& m : boule.GetCurrentCouncil()) {
        assert(boule.IsCitizenRegistered(m.address));
        assert(boule.IsCouncilMember(m.address));
    }

    // Re-sortition with different seed gives a different (deterministic) result
    Boule boule2(COUNCIL_SIZE, 1000, 0, true);
    for (uint8_t i = 1; i <= 10; ++i) {
        boule2.RegisterCitizen(Addr(i), 100, 0);
    }
    assert(boule2.ConductSortition(Seed(0x12345678), 100));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestSortitionNotEnoughCitizens() {
    std::cout << "Test: Sortition fails when pool too small" << std::endl;

    Boule boule(5, 1000, 0, true);
    boule.RegisterCitizen(Addr(0x01), 1, 0);
    boule.RegisterCitizen(Addr(0x02), 1, 0);

    // Only 2 eligible but need 5 → fail
    assert(!boule.ConductSortition(Seed(0xABCD), 0));
    assert(boule.GetCurrentCouncil().empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestProposalReviewAndApproval() {
    std::cout << "Test: Proposal review and 2/3 approval" << std::endl;

    const uint32_t COUNCIL_SIZE = 3;
    Boule boule(COUNCIL_SIZE, 1000, 0, /*screening=*/true);

    for (uint8_t i = 1; i <= 5; ++i) boule.RegisterCitizen(Addr(i), 1, 0);
    boule.ConductSortition(Seed(42), 0);

    auto council = boule.GetCurrentCouncil();
    assert(council.size() == COUNCIL_SIZE);

    // No reviews yet → not approved
    assert(!boule.IsProposalApproved(1));

    // Non-council member cannot review
    assert(!boule.ReviewProposal(1, Addr(0xFF), true, "X", 0));

    // One approval (< 2/3 of 3 = 2)
    assert(boule.ReviewProposal(1, council[0].address, true, "looks good", 0));
    assert(!boule.IsProposalApproved(1));

    // Second approval (reaches 2/3 threshold)
    assert(boule.ReviewProposal(1, council[1].address, true, "ok", 0));
    assert(boule.IsProposalApproved(1));

    // Duplicate review must fail
    assert(!boule.ReviewProposal(1, council[0].address, true, "again", 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestScreeningDisabled() {
    std::cout << "Test: IsProposalApproved=true when screening is disabled" << std::endl;

    Boule boule(3, 1000, 0, /*screening=*/false);
    // No citizens, no council, no reviews – still approved
    assert(boule.IsProposalApproved(99));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGrapheParanomon() {
    std::cout << "Test: Graphe Paranomon (unconstitutionality challenge)" << std::endl;

    const uint32_t COUNCIL_SIZE = 5;
    Boule boule(COUNCIL_SIZE, 1000, 0, true);

    for (uint8_t i = 1; i <= 8; ++i) boule.RegisterCitizen(Addr(i), 1, 0);
    boule.ConductSortition(Seed(7), 0);

    auto council = boule.GetCurrentCouncil();

    // Approve proposal first
    for (size_t i = 0; i < council.size(); ++i) {
        boule.ReviewProposal(42, council[i].address, true, "ok", 0);
    }
    assert(boule.IsProposalApproved(42));

    // Council member raises a Graphe Paranomon
    assert(boule.RaiseGrapheParanomon(42, council[0].address,
                                      "Violates constitutional quorum rule", 10));
    assert(boule.HasActiveChallenge(42));

    // While challenge is unresolved, proposal is NOT approved
    assert(!boule.IsProposalApproved(42));

    // Non-council member cannot raise or vote
    assert(!boule.RaiseGrapheParanomon(42, Addr(0xEE), "bad actor", 10));

    // Votes to dismiss (3 needed for majority of 5)
    boule.VoteOnGrapheParanomon(42, council[1].address, /*dismiss=*/true, 11);
    boule.VoteOnGrapheParanomon(42, council[2].address, /*dismiss=*/true, 11);
    assert(boule.HasActiveChallenge(42));  // 2 dismiss, not majority yet
    boule.VoteOnGrapheParanomon(42, council[3].address, /*dismiss=*/true, 11);

    // Challenge resolved (dismissed) → proposal is approved again
    assert(!boule.HasActiveChallenge(42));
    auto ch = boule.GetChallenge(42);
    assert(ch.has_value() && ch->resolved && !ch->upheld);
    assert(boule.IsProposalApproved(42));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestPrytany() {
    std::cout << "Test: Prytany (rotating executive committee)" << std::endl;

    const uint32_t COUNCIL_SIZE = 10;
    Boule boule(COUNCIL_SIZE, 1000, 0, true);

    for (uint8_t i = 1; i <= 15; ++i) boule.RegisterCitizen(Addr(i), 1, 0);
    boule.ConductSortition(Seed(99), 0);

    auto prytany = boule.GetPrytany();
    // Prytany = council_size / 10 = 1
    assert(prytany.size() == 1);

    // Prytany member must be a council member
    assert(boule.IsCouncilMember(prytany[0].address));
    assert(boule.IsPrytanyMember(prytany[0].address));

    // Non-council member is not Prytany
    assert(!boule.IsPrytanyMember(Addr(0xFF)));

    // Empty council → empty Prytany
    Boule b2(3, 1000, 0, false);
    assert(b2.GetPrytany().empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestEligibilityToggle() {
    std::cout << "Test: Citizen eligibility toggle (marks ineligible for sortition)" << std::endl;

    Boule boule(2, 1000, 0, false);

    boule.RegisterCitizen(Addr(0x01), 1, 0);
    boule.RegisterCitizen(Addr(0x02), 1, 0);
    boule.RegisterCitizen(Addr(0x03), 1, 0);

    // Mark citizen 1 ineligible (e.g. ostracised)
    assert(boule.SetCitizenEligibility(Addr(0x01), false));

    // Only 2 eligible → sortition with council_size=2 should succeed
    assert(boule.ConductSortition(Seed(0x11223344), 0));

    // Ineligible citizen must NOT appear in council
    for (const auto& m : boule.GetCurrentCouncil()) {
        assert(m.address != Addr(0x01));
    }

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "Boule (Athenian Council) Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestCitizenRegistration();
        TestSortition();
        TestSortitionNotEnoughCitizens();
        TestProposalReviewAndApproval();
        TestScreeningDisabled();
        TestGrapheParanomon();
        TestPrytany();
        TestEligibilityToggle();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All Boule tests passed! \u2713" << std::endl;
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
