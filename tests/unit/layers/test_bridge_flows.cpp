#include "common/bridging.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace pantheon::common;

    BridgeTransfer l1_to_l2{BridgeLayer::L1, BridgeLayer::L2, "TALANTON", "alice", 10};
    auto deposit = ValidateDeposit(l1_to_l2);
    assert(deposit.ok);

    BridgeTransfer l2_to_l1{BridgeLayer::L2, BridgeLayer::L1, "wTALANTON", "alice", 5};
    auto early_withdraw = ValidateWithdrawal(l2_to_l1, 120, 115, 10);
    assert(!early_withdraw.ok);

    auto settled_withdraw = ValidateWithdrawal(l2_to_l1, 130, 115, 10);
    assert(settled_withdraw.ok);

    BridgeTransfer invalid_hop{BridgeLayer::L1, BridgeLayer::L3, "TALANTON", "alice", 10};
    auto invalid = ValidateDeposit(invalid_hop);
    assert(!invalid.ok);

    std::cout << "Bridge flow tests passed" << std::endl;
    return 0;
}
