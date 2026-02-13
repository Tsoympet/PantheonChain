#include "chainparams.h"

#include "p2p/protocol.h"

#include <algorithm>
#include <cctype>

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
    const auto first_non_space = mode_name.find_first_not_of(" \t\n\r");
    if (first_non_space == std::string::npos) {
        return std::nullopt;
    }
    const auto last_non_space = mode_name.find_last_not_of(" \t\n\r");

    std::string normalized =
        mode_name.substr(first_non_space, last_non_space - first_non_space + 1);
    std::string normalized = mode_name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (normalized == "mainnet" || normalized == "main" || normalized == "mainet") {
        return NetworkMode::MAINNET;
    }
    if (normalized == "testnet" || normalized == "test") {
        return NetworkMode::TESTNET;
    }
    if (normalized == "regtest" || normalized == "reg") {
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
