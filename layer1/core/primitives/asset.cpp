// ParthenonChain - Asset ID Implementation
// Consensus-critical: Multi-asset UTXO system

#include "asset.h"

namespace parthenon {
namespace primitives {

void AssetAmount::Serialize(uint8_t* output) const {
    // Byte 0: Asset ID
    output[0] = static_cast<uint8_t>(asset);

    // Bytes 1-8: Amount (little-endian)
    output[1] = static_cast<uint8_t>(amount);
    output[2] = static_cast<uint8_t>(amount >> 8);
    output[3] = static_cast<uint8_t>(amount >> 16);
    output[4] = static_cast<uint8_t>(amount >> 24);
    output[5] = static_cast<uint8_t>(amount >> 32);
    output[6] = static_cast<uint8_t>(amount >> 40);
    output[7] = static_cast<uint8_t>(amount >> 48);
    output[8] = static_cast<uint8_t>(amount >> 56);
}

AssetAmount AssetAmount::Deserialize(const uint8_t* input) {
    AssetAmount result;

    // Byte 0: Asset ID
    result.asset = static_cast<AssetID>(input[0]);

    // Bytes 1-8: Amount (little-endian)
    result.amount = 0;
    result.amount |= static_cast<uint64_t>(input[1]);
    result.amount |= static_cast<uint64_t>(input[2]) << 8;
    result.amount |= static_cast<uint64_t>(input[3]) << 16;
    result.amount |= static_cast<uint64_t>(input[4]) << 24;
    result.amount |= static_cast<uint64_t>(input[5]) << 32;
    result.amount |= static_cast<uint64_t>(input[6]) << 40;
    result.amount |= static_cast<uint64_t>(input[7]) << 48;
    result.amount |= static_cast<uint64_t>(input[8]) << 56;

    return result;
}

}  // namespace primitives
}  // namespace parthenon
