#include "consensus/genesis.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace parthenon::consensus;

namespace {

std::array<uint8_t, 32> MakeHash(std::initializer_list<uint8_t> bytes) {
    std::array<uint8_t, 32> hash{};
    assert(bytes.size() == hash.size());
    std::copy(bytes.begin(), bytes.end(), hash.begin());
    return hash;
}

} // namespace

void TestGenesisShape() {
    auto mainnet = GetGenesisBlock(NetworkType::MAINNET);
    assert(mainnet.IsGenesis());
    assert(mainnet.transactions.size() == 1);
    assert(mainnet.transactions[0].IsCoinbase());
    assert(mainnet.transactions[0].outputs.size() == 3);

    // Deterministic outputs from params
    const auto p = GetGenesisParams(NetworkType::MAINNET);
    assert(mainnet.transactions[0].outputs[0].value.amount == p.talanton_output);
    assert(mainnet.transactions[0].outputs[1].value.amount == p.drachma_output);
    assert(mainnet.transactions[0].outputs[2].value.amount == p.obolos_output);
}

void TestNetworkDifferences() {
    auto mainnet = GetGenesisBlock(NetworkType::MAINNET);
    auto testnet = GetGenesisBlock(NetworkType::TESTNET);
    auto regtest = GetGenesisBlock(NetworkType::REGTEST);

    assert(mainnet.header.timestamp == testnet.header.timestamp);
    assert(regtest.header.bits == 0x207fffff);
    assert(mainnet.header.bits == testnet.header.bits);

    // Coinbase message differs by network at least for testnet/regtest.
    assert(mainnet.transactions[0].Serialize() != testnet.transactions[0].Serialize());
    assert(testnet.transactions[0].Serialize() != regtest.transactions[0].Serialize());
}

void TestExpectedGenesisValidation() {
    auto block = GetGenesisBlock(NetworkType::MAINNET);
    assert(IsExpectedGenesisBlock(block, NetworkType::MAINNET));

    // Tamper nonce
    block.header.nonce += 1;
    assert(!IsExpectedGenesisBlock(block, NetworkType::MAINNET));
}

void TestExpectedGenesisHashes() {
    const auto mainnet_expected =
        MakeHash({0x66, 0x01, 0x89, 0xb8, 0x46, 0x6f, 0xa2, 0x95, 0x7f, 0x0a, 0x8b,
                  0xf9, 0xbb, 0xe6, 0xfd, 0xda, 0x5c, 0xc4, 0xeb, 0x79, 0xae, 0x57,
                  0xf2, 0x41, 0x61, 0xdf, 0x72, 0x6c, 0xee, 0x4b, 0x85, 0x44});
    const auto testnet_expected =
        MakeHash({0xfe, 0x73, 0x9e, 0x3c, 0x1e, 0x27, 0x50, 0x9a, 0x9c, 0x7d, 0x22,
                  0x60, 0xe3, 0x94, 0x38, 0xf1, 0xff, 0x1c, 0x44, 0xdc, 0x03, 0xa6,
                  0x30, 0x9e, 0x68, 0x73, 0x7b, 0x2c, 0xc6, 0x2c, 0x38, 0x75});
    const auto regtest_expected =
        MakeHash({0xf8, 0x5d, 0x0b, 0xad, 0x36, 0xaf, 0xae, 0x82, 0xec, 0x8b, 0x2c,
                  0x60, 0xa9, 0x58, 0x7a, 0xb2, 0xd3, 0x65, 0x81, 0xb4, 0xea, 0x31,
                  0xa1, 0x08, 0xb7, 0xcc, 0xfe, 0xfb, 0x97, 0x76, 0xbc, 0x15});

    assert(GetExpectedGenesisHash(NetworkType::MAINNET) == mainnet_expected);
    assert(GetExpectedGenesisHash(NetworkType::TESTNET) == testnet_expected);
    assert(GetExpectedGenesisHash(NetworkType::REGTEST) == regtest_expected);

    assert(GetGenesisHash(NetworkType::MAINNET) == mainnet_expected);
    assert(GetGenesisHash(NetworkType::TESTNET) == testnet_expected);
    assert(GetGenesisHash(NetworkType::REGTEST) == regtest_expected);
}

int main() {
    std::cout << "=== Genesis Tests ===" << std::endl;
    TestGenesisShape();
    TestNetworkDifferences();
    TestExpectedGenesisValidation();
    TestExpectedGenesisHashes();
    std::cout << "\n✓ All genesis tests passed!" << std::endl;
    return 0;
}
