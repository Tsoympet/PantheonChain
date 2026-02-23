// ParthenonChain – GovernanceEventLog Unit Tests

#include "governance/eventlog.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

void TestLogAndSize() {
    std::cout << "Test: Log events and size" << std::endl;

    GovernanceEventLog log;
    assert(log.Size() == 0);

    log.Log(GovernanceEventLog::EventType::PROPOSAL_CREATED, 100,
            Addr(0x01), 1, "created proposal #1");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_VOTE_CAST, 110,
            Addr(0x02), 1, "vote YES");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_PASSED, 200,
            Addr(0x01), 1, "proposal passed");

    assert(log.Size() == 3);

    // Sequential IDs
    const auto& all = log.GetAll();
    assert(all[0].event_id == 1);
    assert(all[1].event_id == 2);
    assert(all[2].event_id == 3);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestFilterByType() {
    std::cout << "Test: GetByType filter" << std::endl;

    GovernanceEventLog log;
    log.Log(GovernanceEventLog::EventType::TREASURY_DEPOSIT, 10, Addr(0x01), 0, "deposit");
    log.Log(GovernanceEventLog::EventType::STAKE_DEPOSITED,  11, Addr(0x02), 0, "stake");
    log.Log(GovernanceEventLog::EventType::TREASURY_DEPOSIT, 12, Addr(0x03), 0, "deposit2");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_CREATED, 13, Addr(0x04), 5, "proposal");

    auto deps = log.GetByType(GovernanceEventLog::EventType::TREASURY_DEPOSIT);
    assert(deps.size() == 2);

    auto stakes = log.GetByType(GovernanceEventLog::EventType::STAKE_DEPOSITED);
    assert(stakes.size() == 1);

    auto none = log.GetByType(GovernanceEventLog::EventType::OSTRACISM_ENACTED);
    assert(none.empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestFilterByActor() {
    std::cout << "Test: GetByActor filter" << std::endl;

    GovernanceEventLog log;
    log.Log(GovernanceEventLog::EventType::PROPOSAL_CREATED, 1, Addr(0xAA), 1, "p1");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_VOTE_CAST, 2, Addr(0xBB), 1, "vote");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_CREATED, 3, Addr(0xAA), 2, "p2");

    auto aa_events = log.GetByActor(Addr(0xAA));
    assert(aa_events.size() == 2);
    assert(aa_events[0].reference_id == 1);
    assert(aa_events[1].reference_id == 2);

    auto bb_events = log.GetByActor(Addr(0xBB));
    assert(bb_events.size() == 1);

    auto cc_events = log.GetByActor(Addr(0xCC));
    assert(cc_events.empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestFilterByBlockRange() {
    std::cout << "Test: GetByBlockRange filter" << std::endl;

    GovernanceEventLog log;
    for (uint64_t b = 100; b <= 500; b += 100) {
        log.Log(GovernanceEventLog::EventType::PARAM_CHANGED, b, Addr(0x01), 0, "param");
    }
    assert(log.Size() == 5);

    auto all = log.GetByBlockRange(0, 1000);
    assert(all.size() == 5);

    auto middle = log.GetByBlockRange(200, 400);
    assert(middle.size() == 3);  // 200, 300, 400

    auto single = log.GetByBlockRange(300, 300);
    assert(single.size() == 1 && single[0].block_height == 300);

    auto none = log.GetByBlockRange(600, 800);
    assert(none.empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestFilterByReferenceId() {
    std::cout << "Test: GetByReferenceId filter" << std::endl;

    GovernanceEventLog log;
    log.Log(GovernanceEventLog::EventType::PROPOSAL_CREATED,  10, Addr(0x01), 7, "created");
    log.Log(GovernanceEventLog::EventType::BOULE_PROPOSAL_REVIEWED, 11, Addr(0x02), 7, "reviewed");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_PASSED,   20, Addr(0x01), 7, "passed");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_EXECUTED, 30, Addr(0x01), 7, "executed");
    log.Log(GovernanceEventLog::EventType::PROPOSAL_CREATED,  10, Addr(0x01), 8, "other prop");

    auto p7 = log.GetByReferenceId(7);
    assert(p7.size() == 4);

    auto p8 = log.GetByReferenceId(8);
    assert(p8.size() == 1);

    auto p99 = log.GetByReferenceId(99);
    assert(p99.empty());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestEventContents() {
    std::cout << "Test: Event field correctness" << std::endl;

    GovernanceEventLog log;
    log.Log(GovernanceEventLog::EventType::STAKE_SLASHED, 555,
            Addr(0xDE), 42, "slashed for governance attack");

    const auto& ev = log.GetAll()[0];
    assert(ev.event_id    == 1);
    assert(ev.type        == GovernanceEventLog::EventType::STAKE_SLASHED);
    assert(ev.block_height == 555);
    assert(ev.actor       == Addr(0xDE));
    assert(ev.reference_id == 42);
    assert(ev.description == "slashed for governance attack");

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "GovernanceEventLog Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestLogAndSize();
        TestFilterByType();
        TestFilterByActor();
        TestFilterByBlockRange();
        TestFilterByReferenceId();
        TestEventContents();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All GovernanceEventLog tests passed! \u2713" << std::endl;
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
