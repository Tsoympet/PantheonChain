#include "genesis.h"

#include "consensus/difficulty.h"
#include "primitives/asset.h"
#include "primitives/transaction.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace consensus {

namespace {

constexpr uint32_t kGenesisTimestamp = 1704067200;  // 2024-01-01 00:00:00 UTC
constexpr uint64_t kTalantonGenesis = 50ULL * primitives::AssetSupply::BASE_UNIT;
constexpr uint64_t kDrachmaGenesis = 9761900000ULL;  // 97.61900000
constexpr uint64_t kObolosGenesis = 14523800000ULL;  // 145.23800000

}  // namespace

GenesisParams GetGenesisParams(NetworkType network) {
    switch (network) {
        case NetworkType::MAINNET:
            return GenesisParams{NetworkType::MAINNET,
                                 "mainnet",
                                 kGenesisTimestamp,
                                 Difficulty::GetInitialBits(),
                                 0,
                                 "ParthenonChain Genesis - 2024-01-01 - The Times 01/Jan/2024",
                                 kTalantonGenesis,
                                 kDrachmaGenesis,
                                 kObolosGenesis};
        case NetworkType::TESTNET:
            return GenesisParams{NetworkType::TESTNET,
                                 "testnet",
                                 kGenesisTimestamp,
                                 Difficulty::GetInitialBits(),
                                 0,
                                 "ParthenonChain Testnet Genesis - 2024-01-01",
                                 kTalantonGenesis,
                                 kDrachmaGenesis,
                                 kObolosGenesis};
        case NetworkType::REGTEST:
            return GenesisParams{NetworkType::REGTEST,
                                 "regtest",
                                 kGenesisTimestamp,
                                 0x207fffff,
                                 0,
                                 "ParthenonChain Regtest Genesis",
                                 kTalantonGenesis,
                                 kDrachmaGenesis,
                                 kObolosGenesis};
    }

    return GetGenesisParams(NetworkType::MAINNET);
}

primitives::Block GetGenesisBlock(NetworkType network) {
    const auto params = GetGenesisParams(network);

    primitives::Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;

    primitives::TxInput input;
    input.prevout = primitives::OutPoint(std::array<uint8_t, 32>{}, primitives::COINBASE_VOUT_INDEX);
    input.signature_script.assign(params.coinbase_message,
                                  params.coinbase_message + std::strlen(params.coinbase_message));
    coinbase.inputs.push_back(input);

    const std::vector<uint8_t> unspendable_script(32, 0x00);
    coinbase.outputs.emplace_back(primitives::AssetID::TALANTON, params.talanton_output,
                                  unspendable_script);
    coinbase.outputs.emplace_back(primitives::AssetID::DRACHMA, params.drachma_output,
                                  unspendable_script);
    coinbase.outputs.emplace_back(primitives::AssetID::OBOLOS, params.obolos_output,
                                  unspendable_script);

    primitives::Block genesis;
    genesis.header.version = 1;
    genesis.header.prev_block_hash.fill(0);
    genesis.header.timestamp = params.timestamp;
    genesis.header.bits = params.bits;
    genesis.header.nonce = params.nonce;
    genesis.transactions.push_back(coinbase);
    genesis.header.merkle_root = genesis.CalculateMerkleRoot();

    return genesis;
}

std::array<uint8_t, 32> GetGenesisHash(NetworkType network) {
    return GetGenesisBlock(network).GetHash();
}

bool IsExpectedGenesisBlock(const primitives::Block& block, NetworkType network) {
    const auto expected = GetGenesisBlock(network);

    if (block.header.version != expected.header.version ||
        block.header.prev_block_hash != expected.header.prev_block_hash ||
        block.header.timestamp != expected.header.timestamp ||
        block.header.bits != expected.header.bits ||
        block.header.nonce != expected.header.nonce ||
        block.header.merkle_root != expected.header.merkle_root) {
        return false;
    }

    if (block.transactions.size() != 1 || !block.transactions[0].IsCoinbase()) {
        return false;
    }

    return block.transactions[0].Serialize() == expected.transactions[0].Serialize();
}

}  // namespace consensus
}  // namespace parthenon
