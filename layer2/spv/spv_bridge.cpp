#include "spv_bridge.h"
#include <algorithm>

namespace parthenon {
namespace layer2 {

bool SPVBridge::VerifyMerkleProof(const MerkleProof& proof,
                                  const std::vector<uint8_t>& merkle_root) {
    if (proof.proof_hashes.size() != proof.proof_flags.size()) {
        return false;
    }
    
    std::vector<uint8_t> current_hash = proof.tx_hash;
    
    for (size_t i = 0; i < proof.proof_hashes.size(); ++i) {
        if (proof.proof_flags[i]) {
            // Proof hash is on the right
            current_hash = HashPair(current_hash, proof.proof_hashes[i]);
        } else {
            // Proof hash is on the left
            current_hash = HashPair(proof.proof_hashes[i], current_hash);
        }
    }
    
    return current_hash == merkle_root;
}

bool SPVBridge::VerifyTransactionInclusion(const parthenon::primitives::Transaction& tx,
                                          const MerkleProof& proof,
                                          const parthenon::primitives::BlockHeader& header) {
    // Compute transaction hash - serialize and hash it
    auto tx_data = tx.Serialize();
    auto hash_arr = parthenon::crypto::SHA256::Hash256(tx_data);
    std::vector<uint8_t> tx_hash(hash_arr.begin(), hash_arr.end());
    
    // Verify hash matches proof
    if (tx_hash != proof.tx_hash) {
        return false;
    }
    
    // Convert merkle_root array to vector
    std::vector<uint8_t> merkle_root_vec(header.merkle_root.begin(), header.merkle_root.end());
    return VerifyMerkleProof(proof, merkle_root_vec);
}

MerkleProof SPVBridge::BuildMerkleProof(const std::vector<uint8_t>& tx_hash,
                                       const std::vector<std::vector<uint8_t>>& tx_hashes) {
    MerkleProof proof;
    proof.tx_hash = tx_hash;
    
    // Find index of transaction
    auto it = std::find(tx_hashes.begin(), tx_hashes.end(), tx_hash);
    if (it == tx_hashes.end()) {
        return proof;  // Empty proof if not found
    }
    
    size_t index = std::distance(tx_hashes.begin(), it);
    std::vector<std::vector<uint8_t>> level = tx_hashes;
    
    while (level.size() > 1) {
        size_t pair_index = index ^ 1;  // XOR with 1 to get sibling
        
        if (pair_index < level.size()) {
            proof.proof_hashes.push_back(level[pair_index]);
            proof.proof_flags.push_back(index % 2 == 0);  // true if we're on left
        }
        
        // Build next level
        std::vector<std::vector<uint8_t>> next_level;
        for (size_t i = 0; i < level.size(); i += 2) {
            if (i + 1 < level.size()) {
                next_level.push_back(HashPair(level[i], level[i + 1]));
            } else {
                next_level.push_back(HashPair(level[i], level[i]));
            }
        }
        
        level = next_level;
        index /= 2;
    }
    
    return proof;
}

std::vector<uint8_t> SPVBridge::ComputeMerkleRoot(const std::vector<std::vector<uint8_t>>& hashes) {
    if (hashes.empty()) {
        return std::vector<uint8_t>(32, 0);
    }
    
    if (hashes.size() == 1) {
        return hashes[0];
    }
    
    std::vector<std::vector<uint8_t>> level = hashes;
    
    while (level.size() > 1) {
        std::vector<std::vector<uint8_t>> next_level;
        for (size_t i = 0; i < level.size(); i += 2) {
            if (i + 1 < level.size()) {
                next_level.push_back(HashPair(level[i], level[i + 1]));
            } else {
                next_level.push_back(HashPair(level[i], level[i]));
            }
        }
        level = next_level;
    }
    
    return level[0];
}

std::vector<uint8_t> SPVBridge::HashPair(const std::vector<uint8_t>& left,
                                        const std::vector<uint8_t>& right) {
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    auto hash1 = parthenon::crypto::SHA256::Hash256(combined);
    std::vector<uint8_t> hash1_vec(hash1.begin(), hash1.end());
    auto hash2 = parthenon::crypto::SHA256::Hash256(hash1_vec);
    return std::vector<uint8_t>(hash2.begin(), hash2.end());
}

} // namespace layer2
} // namespace parthenon
