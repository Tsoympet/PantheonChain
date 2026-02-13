#include "chainparams.h"

#include "p2p/protocol.h"

namespace parthenon {
namespace node {

NetworkParams GetNetworkParams(NetworkMode mode) {
    switch (mode) {
        case NetworkMode::MAINNET:
            return NetworkParams{NetworkMode::MAINNET,
                                 "mainnet",
                                 p2p::NetworkMagic::MAINNET,
                                 8333,
                                 8332,
                                 {{"seed.pantheonchain.io", 8333},
                                  {"seed2.pantheonchain.io", 8333}},
                                 true};
        case NetworkMode::TESTNET:
            return NetworkParams{NetworkMode::TESTNET,
                                 "testnet",
                                 p2p::NetworkMagic::TESTNET,
                                 18333,
                                 18332,
                                 {{"testnet-seed.pantheonchain.io", 18333}},
                                 true};
        case NetworkMode::REGTEST:
            return NetworkParams{NetworkMode::REGTEST,
                                 "regtest",
                                 p2p::NetworkMagic::REGTEST,
                                 18444,
                                 18443,
                                 {},
                                 false};
    }

    // Fallback (should be unreachable, keeps compilers happy).
    return NetworkParams{NetworkMode::MAINNET,
                         "mainnet",
                         p2p::NetworkMagic::MAINNET,
                         8333,
                         8332,
                         {{"seed.pantheonchain.io", 8333},
                          {"seed2.pantheonchain.io", 8333}},
                         true};
}


std::optional<NetworkMode> ParseNetworkMode(const std::string& mode_name) {
    if (mode_name == "mainnet") {
        return NetworkMode::MAINNET;
    }
    if (mode_name == "testnet") {
        return NetworkMode::TESTNET;
    }
    if (mode_name == "regtest") {
        return NetworkMode::REGTEST;
    }
    return std::nullopt;
}

const char* NetworkModeToString(NetworkMode mode) {
    switch (mode) {
        case NetworkMode::MAINNET:
            return "mainnet";
        case NetworkMode::TESTNET:
            return "testnet";
        case NetworkMode::REGTEST:
            return "regtest";
    }
    return "mainnet";
}

}  // namespace node
}  // namespace parthenon
