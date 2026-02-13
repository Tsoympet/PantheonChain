#pragma once

#include "node.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace node {

struct DNSSeed {
    std::string host;
    uint16_t port;
};

struct NetworkParams {
    NetworkMode mode;
    const char* name;
    uint32_t magic;
    uint16_t default_p2p_port;
    uint16_t default_rpc_port;
    std::vector<DNSSeed> dns_seeds;
    bool dns_discovery_enabled;
};

NetworkParams GetNetworkParams(NetworkMode mode);

std::optional<NetworkMode> ParseNetworkMode(const std::string& mode_name);
const char* NetworkModeToString(NetworkMode mode);

}  // namespace node
}  // namespace parthenon
