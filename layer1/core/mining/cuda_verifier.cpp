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
    // TODO: When CUDA support is added, check for CUDA device here
    // For now, return false (CPU-only implementation)
    return false;
}

bool CUDAVerifier::Initialize() {
    if (initialized_) {
        return true;
    }

    // TODO: Initialize CUDA device when CUDA support is added
    // For now, use CPU fallback
    std::cout << "CUDA not available - using CPU fallback for verification" << std::endl;
    initialized_ = true;
    return true;
}

void CUDAVerifier::Shutdown() {
    if (!initialized_) {
        return;
    }

    // TODO: Cleanup CUDA resources when CUDA support is added
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
    
    // TODO: Return actual CUDA device info when CUDA support is added
    return "CPU fallback";
}

uint32_t CUDAVerifier::GetCUDACoreCount() {
    // TODO: Return actual CUDA core count when CUDA support is added
    return 0;
}

}  // namespace mining
}  // namespace parthenon
