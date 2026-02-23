// ParthenonChain – SnapshotRegistry Unit Tests

#include "governance/snapshot.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

void TestCreateAndQuery() {
    std::cout << "Test: CreateSnapshot and GetSnapshotPower" << std::endl;

    SnapshotRegistry reg;
    assert(!reg.HasSnapshot(1));
    assert(reg.SnapshotCount() == 0);

    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> powers = {
        {Addr(0x01), 1000},
        {Addr(0x02), 2000},
        {Addr(0x03), 500},
    };

    assert(reg.CreateSnapshot(1, 500, powers));
    assert(reg.HasSnapshot(1));
    assert(reg.SnapshotCount() == 1);
    assert(reg.GetSnapshotBlock(1) == 500);
    assert(reg.GetSnapshotTotalPower(1) == 3500);

    assert(reg.GetSnapshotPower(1, Addr(0x01)) == 1000);
    assert(reg.GetSnapshotPower(1, Addr(0x02)) == 2000);
    assert(reg.GetSnapshotPower(1, Addr(0x03)) == 500);

    // Address not in snapshot → 0
    assert(reg.GetSnapshotPower(1, Addr(0xFF)) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestDuplicateSnapshotRejected() {
    std::cout << "Test: Cannot create two snapshots for the same proposal" << std::endl;

    SnapshotRegistry reg;
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> p1 = {{Addr(0x01), 100}};
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> p2 = {{Addr(0x02), 200}};

    assert(reg.CreateSnapshot(7, 100, p1));
    assert(!reg.CreateSnapshot(7, 200, p2));  // duplicate

    // Original snapshot unchanged
    assert(reg.GetSnapshotPower(7, Addr(0x01)) == 100);
    assert(reg.GetSnapshotPower(7, Addr(0x02)) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestZeroPowerEntriesSkipped() {
    std::cout << "Test: Zero-power entries are excluded from snapshot" << std::endl;

    SnapshotRegistry reg;
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> powers = {
        {Addr(0x01), 500},
        {Addr(0x02), 0},    // should be excluded
        {Addr(0x03), 300},
    };

    reg.CreateSnapshot(3, 10, powers);

    assert(reg.GetSnapshotTotalPower(3) == 800);
    assert(reg.GetSnapshotPower(3, Addr(0x02)) == 0);

    auto snap = reg.GetSnapshot(3);
    assert(snap.has_value());
    assert(snap->entries.size() == 2);  // 0x02 excluded

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestMultipleProposals() {
    std::cout << "Test: Independent snapshots per proposal" << std::endl;

    SnapshotRegistry reg;

    // Proposal 1: block 100
    reg.CreateSnapshot(1, 100, {{Addr(0x01), 1000}});

    // Proposal 2: block 200 – Addr(0x01) has staked more since block 100
    reg.CreateSnapshot(2, 200, {{Addr(0x01), 1500}, {Addr(0x02), 500}});

    // Proposal 1 still sees the old power (1000, NOT 1500)
    assert(reg.GetSnapshotPower(1, Addr(0x01)) == 1000);
    assert(reg.GetSnapshotPower(2, Addr(0x01)) == 1500);

    // Addr(0x02) not in proposal 1 snapshot → 0
    assert(reg.GetSnapshotPower(1, Addr(0x02)) == 0);
    assert(reg.GetSnapshotPower(2, Addr(0x02)) == 500);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestGetSnapshot() {
    std::cout << "Test: GetSnapshot returns nullopt for missing proposal" << std::endl;

    SnapshotRegistry reg;
    assert(!reg.GetSnapshot(99).has_value());
    assert(reg.GetSnapshotBlock(99) == 0);
    assert(reg.GetSnapshotTotalPower(99) == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestPowerFrozenAtSnapshot() {
    std::cout << "Test: Snapshot power is frozen (does not change after creation)" << std::endl;

    SnapshotRegistry reg;
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> powers = {
        {Addr(0x01), 1000},
    };
    reg.CreateSnapshot(5, 100, powers);

    // Simulate: addr 0x01 stakes more after block 100.
    // Snapshot should still return the frozen value.
    // (Snapshot registry has no way to update entries – immutable by design)
    assert(reg.GetSnapshotPower(5, Addr(0x01)) == 1000);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "SnapshotRegistry Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestCreateAndQuery();
        TestDuplicateSnapshotRejected();
        TestZeroPowerEntriesSkipped();
        TestMultipleProposals();
        TestGetSnapshot();
        TestPowerFrozenAtSnapshot();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All SnapshotRegistry tests passed! \u2713" << std::endl;
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
