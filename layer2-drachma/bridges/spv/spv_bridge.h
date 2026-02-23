#ifndef PANTHEON_LAYER2_SPV_SPV_BRIDGE_H
#define PANTHEON_LAYER2_SPV_SPV_BRIDGE_H

#include "layer1-talanton/core/crypto/sha256.h"
#include "layer1-talanton/core/primitives/block.h"
#include "layer1-talanton/core/primitives/transaction.h"

#include <cstdint>
#include <vector>

namespace parthenon {
namespace layer2 {

struct MerkleProof {
    std::vector<uint8_t> tx_hash;
    std::vector<std::vector<uint8_t>> proof_hashes;
    std::vector<bool> proof_flags;  // true = right, false = left

    MerkleProof() = default;
};

class SPVBridge {
  public:
    SPVBridge() = default;

    static bool VerifyMerkleProof(const MerkleProof& proof,
                                  const std::vector<uint8_t>& merkle_root);

    static bool VerifyTransactionInclusion(const parthenon::primitives::Transaction& tx,
                                           const MerkleProof& proof,
                                           const parthenon::primitives::BlockHeader& header);

    static MerkleProof BuildMerkleProof(const std::vector<uint8_t>& tx_hash,
                                        const std::vector<std::vector<uint8_t>>& tx_hashes);

    static std::vector<uint8_t> ComputeMerkleRoot(const std::vector<std::vector<uint8_t>>& hashes);

  private:
    static std::vector<uint8_t> HashPair(const std::vector<uint8_t>& left,
                                         const std::vector<uint8_t>& right);
};

}  // namespace layer2
}  // namespace parthenon

#endif  // PANTHEON_LAYER2_SPV_SPV_BRIDGE_H
