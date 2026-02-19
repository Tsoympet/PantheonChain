#pragma once

#include <cstdint>
#include <string>

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

}  // namespace pantheon::common
