// ParthenonChain – FeeRouter Unit Tests
//
// Verifies the fee-splitting arithmetic and treasury-deposit integration
// for all six fee sources that fund the PantheonChain governance treasury.

#include "governance/eventlog.h"
#include "governance/fee_router.h"
#include "governance/treasury.h"

#include <cassert>
#include <iostream>

using namespace parthenon::governance;

static std::vector<uint8_t> Addr(uint8_t b) { return std::vector<uint8_t>(32, b); }

// ---------------------------------------------------------------------------
// Helper: check split arithmetic sums to total_fee
// ---------------------------------------------------------------------------
static void AssertSplitExact([[maybe_unused]] const FeeRouter::RouteResult &r) {
    assert(r.producer_amount + r.treasury_amount + r.burn_amount == r.total_fee);
}

// ---------------------------------------------------------------------------
void TestDefaultConfigs() {
    std::cout << "Test: Default split configurations are valid (bps sum to 10000)" << std::endl;

    assert(FeeRouter::DefaultL1Config().IsValid());
    assert(FeeRouter::DefaultL2Config().IsValid());
    assert(FeeRouter::DefaultL3BaseFeeConfig().IsValid());
    assert(FeeRouter::DefaultL3PriorityFeeConfig().IsValid());
    assert(FeeRouter::DefaultBridgeFeeConfig().IsValid());
    assert(FeeRouter::DefaultProtocolFeeConfig().IsValid());

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestL1FeeSplit() {
    std::cout << "Test: L1 UTXO fee split (80/15/5)" << std::endl;

    FeeRouter router; // no treasury attached

    // 10,000 units: 8000 producer, 1500 treasury, 500 burn
    auto r = router.Route(FeeRouter::FeeSource::L1_UTXO, 10000, Addr(0x01), 1);
    AssertSplitExact(r);
    assert(r.producer_amount == 8000);
    assert(r.treasury_amount == 1500);
    assert(r.burn_amount == 500);
    assert(!r.treasury_deposited); // no treasury attached

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestL2ValidatorSplit() {
    std::cout << "Test: L2 validator fee split (70/20/10)" << std::endl;

    FeeRouter router;

    auto r = router.Route(FeeRouter::FeeSource::L2_VALIDATOR, 10000, Addr(0x02), 2);
    AssertSplitExact(r);
    assert(r.producer_amount == 7000);
    assert(r.treasury_amount == 2000);
    assert(r.burn_amount == 1000);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestL3BaseFeeHalfBurn() {
    std::cout << "Test: L3 base fee split (0/50/50 – half burn, half treasury)" << std::endl;

    FeeRouter router;

    // 20,000 OBL base fee: 0 to producer, 10,000 to treasury, 10,000 burned
    auto r = router.Route(FeeRouter::FeeSource::L3_BASE_FEE, 20000, Addr(0x03), 3);
    AssertSplitExact(r);
    assert(r.producer_amount == 0);
    assert(r.treasury_amount == 10000);
    assert(r.burn_amount == 10000);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestL3PriorityFeeAllToProducer() {
    std::cout << "Test: L3 priority tip goes 100% to producer" << std::endl;

    FeeRouter router;

    auto r = router.Route(FeeRouter::FeeSource::L3_PRIORITY_FEE, 5000, Addr(0x04), 4);
    AssertSplitExact(r);
    assert(r.producer_amount == 5000);
    assert(r.treasury_amount == 0);
    assert(r.burn_amount == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestBridgeFeeAllToTreasury() {
    std::cout << "Test: Bridge fee goes 100% to treasury (OPERATIONS track)" << std::endl;

    FeeRouter router;

    auto r = router.Route(FeeRouter::FeeSource::BRIDGE_FEE, 3000, Addr(0x05), 5);
    AssertSplitExact(r);
    assert(r.producer_amount == 0);
    assert(r.treasury_amount == 3000);
    assert(r.burn_amount == 0);
    assert(FeeRouter::DefaultBridgeFeeConfig().treasury_track == Treasury::Track::OPERATIONS);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTreasuryDeposit() {
    std::cout << "Test: Treasury receives correct deposits when attached" << std::endl;

    Treasury treasury(1, 0);
    treasury.AddGuardian(Addr(0xFF));
    FeeRouter router(&treasury);

    // L1: 10,000 total → 1,500 to treasury CORE_DEVELOPMENT
    auto r1 = router.Route(FeeRouter::FeeSource::L1_UTXO, 10000, Addr(0x01), 10);
    assert(r1.treasury_deposited);
    assert(treasury.GetTrackBalance(Treasury::Track::CORE_DEVELOPMENT) == 1500);

    // L2: 5,000 total → 1,000 to treasury OPERATIONS
    auto r2 = router.Route(FeeRouter::FeeSource::L2_VALIDATOR, 5000, Addr(0x02), 11);
    assert(r2.treasury_deposited);
    assert(treasury.GetTrackBalance(Treasury::Track::OPERATIONS) == 1000);

    // L3 base: 8,000 total → 4,000 to treasury GRANTS
    auto r3 = router.Route(FeeRouter::FeeSource::L3_BASE_FEE, 8000, Addr(0x03), 12);
    assert(r3.treasury_deposited);
    assert(treasury.GetTrackBalance(Treasury::Track::GRANTS) == 4000);

    // Total treasury balance = 1500 + 1000 + 4000 = 6500
    assert(treasury.GetTotalBalance() == 6500);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestZeroFeeIsNoOp() {
    std::cout << "Test: Zero fee produces zero amounts" << std::endl;

    FeeRouter router;
    auto r = router.Route(FeeRouter::FeeSource::L1_UTXO, 0, Addr(0x01), 1);
    AssertSplitExact(r);
    assert(r.producer_amount == 0);
    assert(r.treasury_amount == 0);
    assert(r.burn_amount == 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestStatsAccumulation() {
    std::cout << "Test: Statistics accumulate correctly across multiple routes" << std::endl;

    FeeRouter router;
    router.Route(FeeRouter::FeeSource::L1_UTXO, 10000, Addr(0x01), 1);
    router.Route(FeeRouter::FeeSource::L1_UTXO, 20000, Addr(0x02), 2);
    router.Route(FeeRouter::FeeSource::L2_VALIDATOR, 5000, Addr(0x03), 3);

    const auto &l1_stats = router.GetSourceStats(FeeRouter::FeeSource::L1_UTXO);
    assert(l1_stats.route_count == 2);
    assert(l1_stats.total_fees_routed == 30000);
    assert(l1_stats.total_to_producer == 24000); // 80% of 30000
    assert(l1_stats.total_to_treasury == 4500);  // 15% of 30000
    assert(l1_stats.total_burned == 1500);       // 5% of 30000

    const auto &l2_stats = router.GetSourceStats(FeeRouter::FeeSource::L2_VALIDATOR);
    assert(l2_stats.route_count == 1);
    assert(l2_stats.total_fees_routed == 5000);

    auto total = router.GetTotalStats();
    assert(total.total_fees_routed == 35000);
    assert(total.route_count == 3);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestTreasuryRevenueSummary() {
    std::cout << "Test: GetTotalTreasuryRevenue and GetTotalBurned" << std::endl;

    Treasury treasury(1, 0);
    FeeRouter router(&treasury);

    router.Route(FeeRouter::FeeSource::L1_UTXO, 10000, Addr(0x01), 1); // +1500 treasury, +500 burn
    router.Route(FeeRouter::FeeSource::L3_BASE_FEE, 10000, Addr(0x02),
                 2); // +5000 treasury, +5000 burn
    router.Route(FeeRouter::FeeSource::BRIDGE_FEE, 2000, Addr(0x03), 3); // +2000 treasury, +0 burn

    assert(router.GetTotalTreasuryRevenue() == 1500 + 5000 + 2000);
    assert(router.GetTotalBurned() == 500 + 5000 + 0);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestCustomSplitConfig() {
    std::cout << "Test: Custom split configuration overrides default" << std::endl;

    FeeRouter router;

    // Override L1 to Polkadot model: 80% treasury, 20% producer, 0% burn
    FeeRouter::SplitConfig polkadot_style{2000, 8000, 0, Treasury::Track::CORE_DEVELOPMENT};
    assert(polkadot_style.IsValid());
    router.SetSplitConfig(FeeRouter::FeeSource::L1_UTXO, polkadot_style);

    auto r = router.Route(FeeRouter::FeeSource::L1_UTXO, 10000, Addr(0x01), 1);
    AssertSplitExact(r);
    assert(r.producer_amount == 2000);
    assert(r.treasury_amount == 8000);
    assert(r.burn_amount == 0);

    // Retrieve stored config
    const auto &stored = router.GetSplitConfig(FeeRouter::FeeSource::L1_UTXO);
    assert(stored.producer_bps == 2000);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestEventLogIntegration() {
    std::cout << "Test: Event log receives entries on each route call" << std::endl;

    GovernanceEventLog log;
    FeeRouter router(nullptr, &log);

    assert(log.Size() == 0);
    router.Route(FeeRouter::FeeSource::L1_UTXO, 1000, Addr(0x01), 10);
    router.Route(FeeRouter::FeeSource::L3_BASE_FEE, 2000, Addr(0x02), 11);
    assert(log.Size() == 2);

    auto entries = log.GetByType(GovernanceEventLog::EventType::TREASURY_DEPOSIT);
    assert(entries.size() == 2);
    assert(entries[0].block_height == 10);
    assert(entries[1].block_height == 11);

    std::cout << "  \u2713 Passed" << std::endl;
}

void TestRoundingNoLeakage() {
    std::cout << "Test: Rounding leaves no satoshi unaccounted (remainder goes to burn)"
              << std::endl;

    FeeRouter router;

    // 10,001 units with L1 (80/15/5):
    //   producer = 10001 * 8000 / 10000 = 8000
    //   treasury = 10001 * 1500 / 10000 = 1500
    //   burn     = 10001 - 8000 - 1500  = 501
    auto r = router.Route(FeeRouter::FeeSource::L1_UTXO, 10001, Addr(0x01), 1);
    AssertSplitExact(r); // must sum exactly to 10001
    assert(r.producer_amount == 8000);
    assert(r.treasury_amount == 1500);
    assert(r.burn_amount == 501);

    std::cout << "  \u2713 Passed" << std::endl;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "FeeRouter Unit Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;

    try {
        TestDefaultConfigs();
        TestL1FeeSplit();
        TestL2ValidatorSplit();
        TestL3BaseFeeHalfBurn();
        TestL3PriorityFeeAllToProducer();
        TestBridgeFeeAllToTreasury();
        TestTreasuryDeposit();
        TestZeroFeeIsNoOp();
        TestStatsAccumulation();
        TestTreasuryRevenueSummary();
        TestCustomSplitConfig();
        TestEventLogIntegration();
        TestRoundingNoLeakage();

        std::cout << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "All FeeRouter tests passed! \u2713" << std::endl;
        std::cout << "=============================================" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
