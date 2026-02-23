#include "fee_router.h"

namespace parthenon {
namespace governance {

// ---------------------------------------------------------------------------
// Default split configs
// ---------------------------------------------------------------------------

FeeRouter::SplitConfig FeeRouter::DefaultL1Config() {
    // L1 UTXO fees (TALN): 80% to miner, 15% to treasury, 5% burned
    return {8000, 1500, 500, Treasury::Track::CORE_DEVELOPMENT};
}

FeeRouter::SplitConfig FeeRouter::DefaultL2Config() {
    // L2 validator fees (DRM): 70% to validator, 20% to treasury, 10% burned
    return {7000, 2000, 1000, Treasury::Track::OPERATIONS};
}

FeeRouter::SplitConfig FeeRouter::DefaultL3BaseFeeConfig() {
    // L3 EVM base fee (OBL): 0% to producer, 50% to treasury, 50% burned
    // Base fee is never producer revenue (EIP-1559 principle).
    return {0, 5000, 5000, Treasury::Track::GRANTS};
}

FeeRouter::SplitConfig FeeRouter::DefaultL3PriorityFeeConfig() {
    // L3 EVM priority tip (OBL): 100% to block producer, nothing else.
    return {10000, 0, 0, Treasury::Track::UNCATEGORIZED};
}

FeeRouter::SplitConfig FeeRouter::DefaultBridgeFeeConfig() {
    // Bridge fees: 100% to treasury OPERATIONS (bridge subsidises ops)
    return {0, 10000, 0, Treasury::Track::OPERATIONS};
}

FeeRouter::SplitConfig FeeRouter::DefaultProtocolFeeConfig() {
    // Miscellaneous protocol fees: 100% to treasury, uncategorised track
    return {0, 10000, 0, Treasury::Track::UNCATEGORIZED};
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

FeeRouter::FeeRouter(Treasury* treasury, GovernanceEventLog* event_log)
    : treasury_(treasury), event_log_(event_log) {
    // Install defaults
    configs_[FeeSource::L1_UTXO]          = DefaultL1Config();
    configs_[FeeSource::L2_VALIDATOR]     = DefaultL2Config();
    configs_[FeeSource::L3_BASE_FEE]      = DefaultL3BaseFeeConfig();
    configs_[FeeSource::L3_PRIORITY_FEE]  = DefaultL3PriorityFeeConfig();
    configs_[FeeSource::BRIDGE_FEE]       = DefaultBridgeFeeConfig();
    configs_[FeeSource::PROTOCOL_FEE]     = DefaultProtocolFeeConfig();

    // Zero-initialise stats
    for (auto s : {FeeSource::L1_UTXO, FeeSource::L2_VALIDATOR,
                   FeeSource::L3_BASE_FEE, FeeSource::L3_PRIORITY_FEE,
                   FeeSource::BRIDGE_FEE, FeeSource::PROTOCOL_FEE}) {
        stats_[s] = SourceStats{};
    }
}

// ---------------------------------------------------------------------------
// Core routing
// ---------------------------------------------------------------------------

FeeRouter::RouteResult FeeRouter::Route(FeeSource source,
                                        uint64_t total_fee,
                                        const std::vector<uint8_t>& producer_address,
                                        uint64_t block_height) {
    const SplitConfig& cfg = configs_.at(source);

    // Integer split (basis-point arithmetic)
    uint64_t producer_amount = total_fee * cfg.producer_bps / 10000u;
    uint64_t treasury_amount = total_fee * cfg.treasury_bps / 10000u;
    // Assign remainder to burn to avoid any satoshi leakage
    uint64_t burn_amount     = total_fee - producer_amount - treasury_amount;

    // Deposit into treasury if attached
    bool deposited = false;
    if (treasury_ != nullptr && treasury_amount > 0) {
        deposited = treasury_->Deposit(treasury_amount, producer_address,
                                       cfg.treasury_track, block_height);
    }

    // Update stats
    SourceStats& st = stats_[source];
    st.total_fees_routed  += total_fee;
    st.total_to_producer  += producer_amount;
    st.total_to_treasury  += treasury_amount;
    st.total_burned       += burn_amount;
    st.route_count        += 1;

    // Append to event log if attached
    if (event_log_ != nullptr) {
        event_log_->Log(GovernanceEventLog::EventType::TREASURY_DEPOSIT,
                        block_height, producer_address,
                        /*reference_id=*/0,
                        SourceName(source) + " fee routed: producer=" +
                            std::to_string(producer_amount) +
                            " treasury=" + std::to_string(treasury_amount) +
                            " burn=" + std::to_string(burn_amount));
    }

    RouteResult result;
    result.source             = source;
    result.total_fee          = total_fee;
    result.producer_amount    = producer_amount;
    result.treasury_amount    = treasury_amount;
    result.burn_amount        = burn_amount;
    result.treasury_deposited = deposited;
    return result;
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void FeeRouter::SetSplitConfig(FeeSource source, const SplitConfig& cfg) {
    configs_[source] = cfg;
}

const FeeRouter::SplitConfig& FeeRouter::GetSplitConfig(FeeSource source) const {
    return configs_.at(source);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

const FeeRouter::SourceStats& FeeRouter::GetSourceStats(FeeSource source) const {
    return stats_.at(source);
}

FeeRouter::SourceStats FeeRouter::GetTotalStats() const {
    SourceStats total{};
    for (const auto& [src, st] : stats_) {
        total.total_fees_routed += st.total_fees_routed;
        total.total_to_producer += st.total_to_producer;
        total.total_to_treasury += st.total_to_treasury;
        total.total_burned      += st.total_burned;
        total.route_count       += st.route_count;
    }
    return total;
}

uint64_t FeeRouter::GetTotalTreasuryRevenue() const {
    uint64_t total = 0;
    for (const auto& [src, st] : stats_) total += st.total_to_treasury;
    return total;
}

uint64_t FeeRouter::GetTotalBurned() const {
    uint64_t total = 0;
    for (const auto& [src, st] : stats_) total += st.total_burned;
    return total;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string FeeRouter::SourceName(FeeSource s) {
    switch (s) {
        case FeeSource::L1_UTXO:         return "L1_UTXO";
        case FeeSource::L2_VALIDATOR:    return "L2_VALIDATOR";
        case FeeSource::L3_BASE_FEE:     return "L3_BASE_FEE";
        case FeeSource::L3_PRIORITY_FEE: return "L3_PRIORITY_FEE";
        case FeeSource::BRIDGE_FEE:      return "BRIDGE_FEE";
        case FeeSource::PROTOCOL_FEE:    return "PROTOCOL_FEE";
    }
    return "UNKNOWN";
}

}  // namespace governance
}  // namespace parthenon
