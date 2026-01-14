// ParthenonChain - CUDA Mining Verification
// GPU-accelerated mining verification for ParthenonChain

#ifndef PARTHENON_MINING_CUDA_VERIFIER_H
#define PARTHENON_MINING_CUDA_VERIFIER_H

#include <cstdint>
#include <vector>

namespace parthenon {
namespace mining {

/**
 * CUDA-based mining verification
 * Provides GPU-accelerated verification of proof-of-work
 */
class CUDAVerifier {
  public:
    /**
     * Check if CUDA is available on this system
     * @return true if CUDA device is available
     */
    static bool IsCUDAAvailable();

    /**
     * Initialize CUDA device for verification
     * @return true if initialization successful
     */
    static bool Initialize();

    /**
     * Shutdown CUDA device
     */
    static void Shutdown();

    /**
     * Verify block hash meets difficulty target using CUDA
     * @param block_header Block header bytes
     * @param target Difficulty target
     * @return true if hash meets target
     */
    static bool VerifyBlockHash(const std::vector<uint8_t>& block_header,
                               const std::vector<uint8_t>& target);

    /**
     * Batch verify multiple block hashes using CUDA
     * More efficient than verifying individually
     * @param block_headers Vector of block headers
     * @param targets Vector of difficulty targets
     * @return Vector of verification results (true/false for each)
     */
    static std::vector<bool> BatchVerify(
        const std::vector<std::vector<uint8_t>>& block_headers,
        const std::vector<std::vector<uint8_t>>& targets);

    /**
     * Get CUDA device information
     * @return String describing CUDA device
     */
    static std::string GetDeviceInfo();

    /**
     * Get number of CUDA cores available
     * @return Number of CUDA cores, or 0 if not available
     */
    static uint32_t GetCUDACoreCount();

  private:
    static bool initialized_;
    static int device_id_;
};

}  // namespace mining
}  // namespace parthenon

#endif  // PARTHENON_MINING_CUDA_VERIFIER_H
