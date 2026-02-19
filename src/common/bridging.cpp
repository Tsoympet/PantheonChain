#include "bridging.h"

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
