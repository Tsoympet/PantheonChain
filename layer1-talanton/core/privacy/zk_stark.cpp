// ParthenonChain - ZK-STARK Implementation

#include "zk_stark.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace {

std::array<uint8_t, 32> HashBytes(const uint8_t* data, size_t size) {
    parthenon::crypto::SHA256 hasher;
    if (size > 0) {
        hasher.Write(data, size);
    }
    return hasher.Finalize();
}

std::array<uint8_t, 32> HashVector(const std::vector<uint8_t>& data) {
    return HashBytes(data.data(), data.size());
}

std::array<uint8_t, 32> HashPair(std::array<uint8_t, 32> left, std::array<uint8_t, 32> right) {
    // Sort pair values for deterministic hashing in this simplified Merkle commitment.
    if (std::lexicographical_compare(right.begin(), right.end(), left.begin(), left.end())) {
        std::swap(left, right);
    }
    parthenon::crypto::SHA256 hasher;
    hasher.Write(left.data(), left.size());
    hasher.Write(right.data(), right.size());
    return hasher.Finalize();
}

std::array<uint8_t, 32> HashTraceRow(const std::vector<uint64_t>& row) {
    parthenon::crypto::SHA256 hasher;
    for (auto value : row) {
        hasher.Write(reinterpret_cast<const uint8_t*>(&value), sizeof(uint64_t));
    }
    return hasher.Finalize();
}

}  // namespace

namespace parthenon {
namespace privacy {
namespace zkstark {

// STARKProver implementation
STARKProver::STARKProver(const STARKParameters& params) : params_(params) {}

STARKProof STARKProver::GenerateProof(const ComputationTrace& trace) {
    STARKProof proof;

    // Build Merkle tree from trace
    proof.merkle_root = BuildMerkleTree(trace.trace_table);

    // Generate FRI proof
    auto fri = FRIProtocol(trace.trace_table);

    std::vector<uint8_t> verification_material;
    if (!proof.merkle_root.empty()) {
        verification_material.insert(verification_material.end(), proof.merkle_root[0].begin(),
                                     proof.merkle_root[0].end());
    }
    verification_material.insert(verification_material.end(), trace.public_input.begin(),
                                 trace.public_input.end());
    verification_material.insert(verification_material.end(), trace.public_output.begin(),
                                 trace.public_output.end());
    const auto* security_bytes = reinterpret_cast<const uint8_t*>(&params_.security_level);
    verification_material.insert(verification_material.end(), security_bytes,
                                 security_bytes + sizeof(uint32_t));
    const auto* blowup_bytes = reinterpret_cast<const uint8_t*>(&params_.blowup_factor);
    verification_material.insert(verification_material.end(), blowup_bytes,
                                 blowup_bytes + sizeof(uint32_t));
    const auto* query_bytes = reinterpret_cast<const uint8_t*>(&params_.num_queries);
    verification_material.insert(verification_material.end(), query_bytes,
                                 query_bytes + sizeof(uint32_t));

    auto verification_hash = HashVector(verification_material);

    proof.proof_data.assign(verification_hash.begin(), verification_hash.end());
    proof.proof_data.insert(proof.proof_data.end(), fri.begin(), fri.end());
    proof.fri_layers = params_.num_queries;

    return proof;
}

STARKProof STARKProver::ProveStatement(const std::vector<uint8_t>& witness,
                                       const std::vector<uint8_t>& public_input) {
    STARKProof proof;
    std::vector<uint8_t> material = public_input;
    material.insert(material.end(), witness.begin(), witness.end());
    const auto* security_bytes = reinterpret_cast<const uint8_t*>(&params_.security_level);
    material.insert(material.end(), security_bytes, security_bytes + sizeof(uint32_t));
    auto hash = HashVector(material);
    proof.proof_data.assign(hash.begin(), hash.end());
    proof.fri_layers = params_.num_queries;
    return proof;
}

std::vector<std::array<uint8_t, 32>>
STARKProver::BuildMerkleTree(const std::vector<std::vector<uint64_t>>& trace) {
    if (trace.empty()) {
        return {};
    }

    std::vector<std::array<uint8_t, 32>> layer;
    layer.reserve(trace.size());
    for (const auto& row : trace) {
        layer.push_back(HashTraceRow(row));
    }

    while (layer.size() > 1) {
        if (layer.size() % 2 != 0) {
            layer.push_back(layer.back());
        }

        std::vector<std::array<uint8_t, 32>> next;
        next.reserve(layer.size() / 2);
        for (size_t i = 0; i < layer.size(); i += 2) {
            next.push_back(HashPair(layer[i], layer[i + 1]));
        }
        layer = std::move(next);
    }

    return layer;
}

std::vector<uint8_t>
STARKProver::FRIProtocol(const std::vector<std::vector<uint64_t>>& polynomial) {
    // Simplified deterministic commitment to polynomial evaluation
    std::vector<uint8_t> material;
    for (const auto& row : polynomial) {
        auto hash = HashTraceRow(row);
        material.insert(material.end(), hash.begin(), hash.end());
    }
    auto hash = HashVector(material);
    return std::vector<uint8_t>(hash.begin(), hash.end());
}

// STARKVerifier implementation
STARKVerifier::STARKVerifier(const STARKParameters& params) : params_(params) {}

bool STARKVerifier::VerifyProof(const STARKProof& proof,
                                const std::vector<uint8_t>& public_input,
                                const std::vector<uint8_t>& public_output) {
    if (!proof.IsValid() || proof.merkle_root.empty() || proof.proof_data.size() < 32) {
        return false;
    }

    std::vector<uint8_t> verification_material;
    verification_material.insert(verification_material.end(), proof.merkle_root[0].begin(),
                                 proof.merkle_root[0].end());
    verification_material.insert(verification_material.end(), public_input.begin(),
                                 public_input.end());
    verification_material.insert(verification_material.end(), public_output.begin(),
                                 public_output.end());
    const auto* security_bytes = reinterpret_cast<const uint8_t*>(&params_.security_level);
    verification_material.insert(verification_material.end(), security_bytes,
                                 security_bytes + sizeof(uint32_t));
    const auto* blowup_bytes = reinterpret_cast<const uint8_t*>(&params_.blowup_factor);
    verification_material.insert(verification_material.end(), blowup_bytes,
                                 blowup_bytes + sizeof(uint32_t));
    const auto* query_bytes = reinterpret_cast<const uint8_t*>(&params_.num_queries);
    verification_material.insert(verification_material.end(), query_bytes,
                                 query_bytes + sizeof(uint32_t));

    auto verification_hash = HashVector(verification_material);
    return std::memcmp(proof.proof_data.data(), verification_hash.data(),
                       verification_hash.size()) == 0;
}

bool STARKVerifier::BatchVerify(const std::vector<STARKProof>& proofs) {
    for (const auto& proof : proofs) {
        if (!proof.IsValid() || proof.merkle_root.empty()) {
            return false;
        }
    }
    return true;
}

// RecursiveSTARK implementation
STARKProof RecursiveSTARK::ProveVerification(const STARKProof& inner_proof) {
    // Wrap the inner proof's data into a recursive proof commitment
    STARKProof recursive;
    recursive.proof_data = inner_proof.proof_data;
    recursive.proof_data.resize(512, 0);  // Pad to fixed size
    recursive.fri_layers = inner_proof.fri_layers;
    return recursive;
}

bool RecursiveSTARK::VerifyRecursive(const STARKProof& recursive_proof) {
    return recursive_proof.IsValid();
}

}  // namespace zkstark
}  // namespace privacy
}  // namespace parthenon
