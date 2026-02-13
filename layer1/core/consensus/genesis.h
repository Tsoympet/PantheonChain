// ParthenonChain - Genesis block definitions
#pragma once

#include "primitives/block.h"

#include <array>
#include <cstdint>
#include <string>

namespace parthenon {
namespace consensus {

enum class NetworkType {
    MAINNET,
    TESTNET,
    REGTEST,
};

struct GenesisParams {
    NetworkType network;
    const char* name;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
    const char* coinbase_message;
    uint64_t talanton_output;
    uint64_t drachma_output;
    uint64_t obolos_output;
};

GenesisParams GetGenesisParams(NetworkType network);
primitives::Block GetGenesisBlock(NetworkType network);
std::array<uint8_t, 32> GetGenesisHash(NetworkType network);
bool IsExpectedGenesisBlock(const primitives::Block& block, NetworkType network);

}  // namespace consensus
}  // namespace parthenon
