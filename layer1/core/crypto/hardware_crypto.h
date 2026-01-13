// ParthenonChain - Hardware-Accelerated Cryptography
// AES-NI and GPU acceleration for cryptographic operations

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <string>

namespace parthenon {
namespace crypto {

/**
 * Hardware-accelerated AES encryption using AES-NI instructions
 */
class HardwareAES {
public:
    /**
     * Initialize with 256-bit key
     */
    bool Init(const std::array<uint8_t, 32>& key);
    
    /**
     * Encrypt data using AES-NI (Intel)
     * @param plaintext Input data
     * @param ciphertext Output buffer (must be at least plaintext.size())
     * @return true if successful
     */
    bool Encrypt(const std::vector<uint8_t>& plaintext, std::vector<uint8_t>& ciphertext);
    
    /**
     * Decrypt data using AES-NI
     */
    bool Decrypt(const std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& plaintext);
    
    /**
     * Check if AES-NI is available on this CPU
     */
    static bool IsAvailable();
    
private:
    std::array<uint8_t, 32> key_;
    bool initialized_ = false;
};

/**
 * GPU-accelerated signature verification using CUDA/OpenCL
 * Batch verification of Schnorr signatures for maximum throughput
 */
class GPUSignatureVerifier {
public:
    /**
     * Initialize GPU context
     * @param device_id GPU device ID (0 for first GPU)
     */
    bool Init(int device_id = 0);
    
    /**
     * Batch verify signatures on GPU
     * @param messages Array of message hashes
     * @param pubkeys Array of public keys
     * @param signatures Array of signatures
     * @param results Output array of verification results
     * @return true if batch completed successfully
     */
    bool BatchVerify(
        const std::vector<std::array<uint8_t, 32>>& messages,
        const std::vector<std::array<uint8_t, 33>>& pubkeys,
        const std::vector<std::array<uint8_t, 64>>& signatures,
        std::vector<bool>& results
    );
    
    /**
     * Get GPU device information
     */
    std::string GetDeviceInfo();
    
    /**
     * Check if GPU acceleration is available
     */
    static bool IsAvailable();
    
    /**
     * Get optimal batch size for this GPU
     */
    size_t GetOptimalBatchSize();
    
    void Shutdown();
    
private:
    void* gpu_context_ = nullptr;
    int device_id_ = -1;
    size_t optimal_batch_size_ = 1024;
};

} // namespace crypto
} // namespace parthenon
