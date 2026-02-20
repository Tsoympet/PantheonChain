#pragma once

#include <string>
#include <vector>

namespace pantheon::common {

struct PeerMessage {
    std::string topic;
    std::string payload;
};

class P2PNetwork {
  public:
    void Broadcast(const PeerMessage& message);
    const std::vector<PeerMessage>& Outbox() const;

  private:
    std::vector<PeerMessage> outbox_;
};

}  // namespace pantheon::common
