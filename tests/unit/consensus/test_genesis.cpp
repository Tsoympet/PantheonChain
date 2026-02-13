#include "consensus/genesis.h"

#include <cassert>
#include <iostream>

using namespace parthenon::consensus;

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

int main() {
    std::cout << "=== Genesis Tests ===" << std::endl;
    TestGenesisShape();
    TestNetworkDifferences();
    TestExpectedGenesisValidation();
    std::cout << "\n✓ All genesis tests passed!" << std::endl;
    return 0;
}
