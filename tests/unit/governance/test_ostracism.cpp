// ParthenonChain – Ostracism Unit Tests

#include "governance/ostracism.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

void TestNominate() {
    std::cout << "Test: Nominate" << std::endl;

    Ostracism os(1000, 3);

    assert(os.Nominate(Addr(0xBB), Addr(0xAA), "governance attack", 0));
    assert(os.IsNominated(Addr(0xBB)));

    // Cannot nominate self
    assert(!os.Nominate(Addr(0xCC), Addr(0xCC), "self", 0));

    // Cannot re-nominate an active nomination
    assert(!os.Nominate(Addr(0xBB), Addr(0xAA), "again", 10));

    // Empty addresses must fail
    assert(!os.Nominate({}, Addr(0xAA), "empty", 0));
    assert(!os.Nominate(Addr(0xAA), {}, "empty", 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestVoting() {
    std::cout << "Test: Vote on ostracism nomination" << std::endl;

    Ostracism os(1000, 3);
    os.Nominate(Addr(0x01), Addr(0x02), "bad actor", 0);

    // Vote FOR
    assert(os.Vote(Addr(0x01), Addr(0x10), true, 0));
    assert(os.HasVoted(Addr(0x01), Addr(0x10)));

    // Duplicate vote must fail
    assert(!os.Vote(Addr(0x01), Addr(0x10), false, 0));

    // Vote AGAINST
    assert(os.Vote(Addr(0x01), Addr(0x20), false, 0));

    // Voting on non-existent nomination must fail
    assert(!os.Vote(Addr(0xFF), Addr(0x30), true, 0));

    auto rec = os.GetRecord(Addr(0x01));
    assert(rec.has_value());
    assert(rec->votes_for    == 1);
    assert(rec->votes_against == 1);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestFinaliseAndBan() {
    std::cout << "Test: Finalize ostracism and apply ban" << std::endl;

    Ostracism os(500, 3);  // ban_duration=500, need 3 votes
    os.Nominate(Addr(0x01), Addr(0x02), "whale manipulation", 100);

    // Finalize before reaching threshold must fail
    assert(os.Vote(Addr(0x01), Addr(0xA1), true, 100));
    assert(os.Vote(Addr(0x01), Addr(0xA2), true, 100));
    assert(!os.Finalize(Addr(0x01), 100));  // only 2 votes

    // Third vote reaches threshold
    assert(os.Vote(Addr(0x01), Addr(0xA3), true, 100));
    assert(os.Finalize(Addr(0x01), 100));

    // Cannot finalize twice
    assert(!os.Finalize(Addr(0x01), 100));

    // IsOstracized must be true during ban window
    assert(os.IsOstracized(Addr(0x01), 100));
    assert(os.IsOstracized(Addr(0x01), 599));
    assert(!os.IsOstracized(Addr(0x01), 600));  // ban ends at 100+500=600

    // Non-ostracized address
    assert(!os.IsOstracized(Addr(0xFF), 100));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestRehabilitation() {
    std::cout << "Test: Rehabilitation after ban expires" << std::endl;

    Ostracism os(200, 1);
    os.Nominate(Addr(0x05), Addr(0x06), "spam proposals", 0);
    os.Vote(Addr(0x05), Addr(0x07), true, 0);
    os.Finalize(Addr(0x05), 0);

    // Cannot rehabilitate during ban
    assert(!os.Rehabilitate(Addr(0x05), 100));

    // Can rehabilitate after ban expires
    assert(os.Rehabilitate(Addr(0x05), 200));

    auto rec = os.GetRecord(Addr(0x05));
    assert(rec.has_value());
    assert(rec->state == Ostracism::State::REHABILITATED);

    // Can now be nominated again (new offence)
    assert(os.Nominate(Addr(0x05), Addr(0x06), "new offence", 300));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetRecordNotFound() {
    std::cout << "Test: GetRecord returns nullopt for unknown address" << std::endl;

    Ostracism os(100, 5);
    assert(!os.GetRecord(Addr(0xFF)).has_value());
    assert(!os.IsNominated(Addr(0xFF)));
    assert(!os.IsOstracized(Addr(0xFF), 0));

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestConfigAccessors() {
    std::cout << "Test: Configuration accessors" << std::endl;

    Ostracism os(300, 7);
    assert(os.GetBanDuration()   == 300);
    assert(os.GetRequiredVotes() == 7);

    os.SetBanDuration(600);
    os.SetRequiredVotes(15);
    assert(os.GetBanDuration()   == 600);
    assert(os.GetRequiredVotes() == 15);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "Ostracism Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestNominate();
        TestVoting();
        TestFinaliseAndBan();
        TestRehabilitation();
        TestGetRecordNotFound();
        TestConfigAccessors();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All Ostracism tests passed! \u2713" << std::endl;
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
