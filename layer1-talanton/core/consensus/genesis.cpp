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

constexpr std::array<uint8_t, 32> kMainnetGenesisHash = {
    0x66, 0x01, 0x89, 0xb8, 0x46, 0x6f, 0xa2, 0x95, 0x7f, 0x0a, 0x8b,
    0xf9, 0xbb, 0xe6, 0xfd, 0xda, 0x5c, 0xc4, 0xeb, 0x79, 0xae, 0x57,
    0xf2, 0x41, 0x61, 0xdf, 0x72, 0x6c, 0xee, 0x4b, 0x85, 0x44};
constexpr std::array<uint8_t, 32> kTestnetGenesisHash = {
    0xfe, 0x73, 0x9e, 0x3c, 0x1e, 0x27, 0x50, 0x9a, 0x9c, 0x7d, 0x22,
    0x60, 0xe3, 0x94, 0x38, 0xf1, 0xff, 0x1c, 0x44, 0xdc, 0x03, 0xa6,
    0x30, 0x9e, 0x68, 0x73, 0x7b, 0x2c, 0xc6, 0x2c, 0x38, 0x75};
constexpr std::array<uint8_t, 32> kRegtestGenesisHash = {
    0xf8, 0x5d, 0x0b, 0xad, 0x36, 0xaf, 0xae, 0x82, 0xec, 0x8b, 0x2c,
    0x60, 0xa9, 0x58, 0x7a, 0xb2, 0xd3, 0x65, 0x81, 0xb4, 0xea, 0x31,
    0xa1, 0x08, 0xb7, 0xcc, 0xfe, 0xfb, 0x97, 0x76, 0xbc, 0x15};

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

std::array<uint8_t, 32> GetExpectedGenesisHash(NetworkType network) {
    switch (network) {
        case NetworkType::MAINNET:
            return kMainnetGenesisHash;
        case NetworkType::TESTNET:
            return kTestnetGenesisHash;
        case NetworkType::REGTEST:
            return kRegtestGenesisHash;
    }

    return kMainnetGenesisHash;
}

bool IsExpectedGenesisBlock(const primitives::Block& block, NetworkType network) {
    const auto expected = GetGenesisBlock(network);
    if (block.GetHash() != GetExpectedGenesisHash(network)) {
        return false;
    }

    if (expected.GetHash() != GetExpectedGenesisHash(network)) {
        return false;
    }

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
