#include "node/chainparams.h"

#include <cassert>
#include <iostream>

using namespace parthenon::node;

void TestMainnetParams() {
    const auto p = GetNetworkParams(NetworkMode::MAINNET);
    assert(p.mode == NetworkMode::MAINNET);
    assert(std::string(p.name) == "mainnet");
    assert(p.magic == 0xD9B4BEF9u);
    assert(p.default_p2p_port == 8333);
    assert(p.default_rpc_port == 8332);
    assert(p.dns_discovery_enabled);
    assert(!p.dns_seeds.empty());
}

void TestRegtestParams() {
    const auto p = GetNetworkParams(NetworkMode::REGTEST);
    assert(p.mode == NetworkMode::REGTEST);
    assert(std::string(p.name) == "regtest");
    assert(p.magic == 0xDAB5BFFAu);
    assert(p.default_p2p_port == 18444);
    assert(p.default_rpc_port == 18443);
    assert(!p.dns_discovery_enabled);
    assert(p.dns_seeds.empty());
}

void TestNetworkModeParsing() {
    assert(ParseNetworkMode("mainnet").has_value());
    assert(ParseNetworkMode("testnet").has_value());
    assert(ParseNetworkMode("regtest").has_value());
    assert(!ParseNetworkMode("MAINNET").has_value());
    assert(!ParseNetworkMode("unknown").has_value());

    assert(std::string(NetworkModeToString(NetworkMode::MAINNET)) == "mainnet");
    assert(std::string(NetworkModeToString(NetworkMode::TESTNET)) == "testnet");
    assert(std::string(NetworkModeToString(NetworkMode::REGTEST)) == "regtest");
}

int main() {
    std::cout << "=== Chain Params Tests ===" << std::endl;
    TestMainnetParams();
    TestRegtestParams();
    TestNetworkModeParsing();
    std::cout << "\n✓ All chain parameter tests passed!" << std::endl;
    return 0;
}
