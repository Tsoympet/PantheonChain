#pragma once

#include <cstdint>
#include <string>

#include <optional>

namespace pantheon::common {

enum class BridgeLayer {
    L1,
    L2,
    L3,
};

struct BridgeTransfer {
    BridgeLayer source_layer;
    BridgeLayer target_layer;
    std::string asset;
    std::string account;
    uint64_t amount = 0;
};

struct BridgeResult {
    bool ok = false;
    std::string reason;
};

BridgeResult ValidateDeposit(const BridgeTransfer& transfer);

BridgeResult ValidateWithdrawal(const BridgeTransfer& transfer,
                               uint64_t current_height,
                               uint64_t finalized_height,
                               uint64_t optimistic_window);


struct GasBudgetQuote {
    uint64_t amount_dr_raw = 0;
    uint64_t amount_ob_raw = 0;
};

std::optional<GasBudgetQuote> EstimateGasBudgetInDr(uint64_t amount_dr_raw);

std::string BuildBridgeAccountingView(const BridgeTransfer& transfer);

}  // namespace pantheon::common
