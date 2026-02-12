// ParthenonChain - CUDA Mining Verification Implementation
// CPU fallback implementation (CUDA optional)

#include "cuda_verifier.h"
#include "crypto/sha256.h"

#include <algorithm>
#include <iostream>

namespace parthenon {
namespace mining {

// Static member initialization
bool CUDAVerifier::initialized_ = false;
int CUDAVerifier::device_id_ = -1;

bool CUDAVerifier::IsCUDAAvailable() {
    // Deterministic CPU backend is always available in this build.
    return true;
}

bool CUDAVerifier::Initialize() {
    if (initialized_) {
        return true;
    }

    device_id_ = 0;
    std::cout << "CUDA backend unavailable - using deterministic CPU verifier" << std::endl;
    initialized_ = true;
    return true;
}

void CUDAVerifier::Shutdown() {
    if (!initialized_) {
        return;
    }

    initialized_ = false;
    device_id_ = -1;
}

bool CUDAVerifier::VerifyBlockHash(const std::vector<uint8_t>& block_header,
                                  const std::vector<uint8_t>& target) {
    // CPU fallback implementation using SHA256
    auto hash = crypto::SHA256::Hash256(block_header);
    
    // Reverse hash for comparison (little-endian)
    std::vector<uint8_t> hash_vec(hash.begin(), hash.end());
    std::reverse(hash_vec.begin(), hash_vec.end());
    
    // Compare hash with target (hash must be less than or equal to target)
    for (size_t i = 0; i < std::min(hash_vec.size(), target.size()); i++) {
        if (hash_vec[i] < target[i]) {
            return true;  // Hash meets target
        } else if (hash_vec[i] > target[i]) {
            return false;  // Hash doesn't meet target
        }
        // If equal, continue to next byte
    }
    
    return true;  // Hashes are equal
}

std::vector<bool> CUDAVerifier::BatchVerify(
    const std::vector<std::vector<uint8_t>>& block_headers,
    const std::vector<std::vector<uint8_t>>& targets) {
    
    std::vector<bool> results;
    results.reserve(block_headers.size());
    
    if (targets.empty()) {
        results.assign(block_headers.size(), false);
        return results;
    }

    // CPU fallback: verify each block individually
    for (size_t i = 0; i < block_headers.size(); i++) {
        const auto& target = (i < targets.size()) ? targets[i] : targets.back();
        results.push_back(VerifyBlockHash(block_headers[i], target));
    }
    
    return results;
}

std::string CUDAVerifier::GetDeviceInfo() {
    if (!IsCUDAAvailable()) {
        return "CPU (CUDA not available)";
    }
    
    return "Deterministic CPU verifier backend";
}

uint32_t CUDAVerifier::GetCUDACoreCount() {
    return 1;
}

}  // namespace mining
}  // namespace parthenon
