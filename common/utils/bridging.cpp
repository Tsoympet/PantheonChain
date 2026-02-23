#include "bridging.h"

#include "common/monetary/units.h"

namespace pantheon::common {

namespace {
bool IsSupportedHop(BridgeLayer source, BridgeLayer target) {
    return (source == BridgeLayer::L1 && target == BridgeLayer::L2) ||
           (source == BridgeLayer::L2 && target == BridgeLayer::L1) ||
           (source == BridgeLayer::L2 && target == BridgeLayer::L3) ||
           (source == BridgeLayer::L3 && target == BridgeLayer::L2);
}
}  // namespace

BridgeResult ValidateDeposit(const BridgeTransfer& transfer) {
    if (!IsSupportedHop(transfer.source_layer, transfer.target_layer)) {
        return {false, "unsupported bridge hop"};
    }
    if (transfer.amount == 0) {
        return {false, "bridge amount must be non-zero"};
    }
    if (transfer.asset.empty() || transfer.account.empty()) {
        return {false, "asset and account are required"};
    }

    return {true, ""};
}

BridgeResult ValidateWithdrawal(const BridgeTransfer& transfer,
                               uint64_t current_height,
                               uint64_t finalized_height,
                               uint64_t optimistic_window) {
    auto base = ValidateDeposit(transfer);
    if (!base.ok) {
        return base;
    }

    if (current_height < finalized_height) {
        return {false, "current height cannot be behind finalized height"};
    }

    if (current_height - finalized_height < optimistic_window) {
        return {false, "withdrawal is still in optimistic trust window"};
    }

    return {true, ""};
}

}  // namespace pantheon::common


namespace pantheon::common {

std::optional<GasBudgetQuote> EstimateGasBudgetInDr(uint64_t amount_dr_raw) {
    auto ob = parthenon::common::monetary::ConvertDrToOb(amount_dr_raw);
    if (!ob) {
        return std::nullopt;
    }
    return GasBudgetQuote{amount_dr_raw, *ob};
}

std::string BuildBridgeAccountingView(const BridgeTransfer& transfer) {
    if (transfer.source_layer == BridgeLayer::L1 && transfer.target_layer == BridgeLayer::L2 &&
        transfer.asset == "TALANTON") {
        auto dr = parthenon::common::monetary::ConvertTalToDr(transfer.amount);
        if (!dr) {
            return "wTAL remains distinct on L2 (overflow computing informational DRACHMA view)";
        }
        return "wTAL remains distinct on L2; informational equivalent=" +
               parthenon::common::monetary::FormatAmount(*dr,
                   parthenon::primitives::AssetID::DRACHMA) + " DRACHMA";
    }

    if (transfer.source_layer == BridgeLayer::L2 && transfer.target_layer == BridgeLayer::L3 &&
        transfer.asset == "DRACHMA") {
        auto ob = EstimateGasBudgetInDr(transfer.amount);
        if (!ob) {
            return "Explicit DRACHMA->OBOLOS conversion required before execution (overflow)";
        }
        return "Explicit DRACHMA->OBOLOS conversion helper: " +
               parthenon::common::monetary::FormatAmount(ob->amount_ob_raw,
                   parthenon::primitives::AssetID::OBOLOS) + " OBOLOS";
    }

    return "No implicit asset conversion. Conversions are explicit or informational only.";
}

}  // namespace pantheon::common
