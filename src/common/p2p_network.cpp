#include "p2p_network.h"

namespace pantheon::common {

void P2PNetwork::Broadcast(const PeerMessage& message) {
    outbox_.push_back(message);
}

const std::vector<PeerMessage>& P2PNetwork::Outbox() const {
    return outbox_;
}

}  // namespace pantheon::common
