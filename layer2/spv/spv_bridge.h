#ifndef PANTHEON_LAYER2_SPV_SPV_BRIDGE_H
#define PANTHEON_LAYER2_SPV_SPV_BRIDGE_H

#include "layer1/core/primitives/transaction.h"
#include "layer1/core/primitives/block.h"
#include "layer1/core/crypto/sha256.h"
#include <vector>
#include <cstdint>

namespace pantheon {
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
    
    static bool VerifyTransactionInclusion(const Transaction& tx,
                                          const MerkleProof& proof,
                                          const BlockHeader& header);
    
    static MerkleProof BuildMerkleProof(const std::vector<uint8_t>& tx_hash,
                                       const std::vector<std::vector<uint8_t>>& tx_hashes);
    
    static std::vector<uint8_t> ComputeMerkleRoot(const std::vector<std::vector<uint8_t>>& hashes);
    
private:
    static std::vector<uint8_t> HashPair(const std::vector<uint8_t>& left,
                                        const std::vector<uint8_t>& right);
};

} // namespace layer2
} // namespace pantheon

#endif // PANTHEON_LAYER2_SPV_SPV_BRIDGE_H
