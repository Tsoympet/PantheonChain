// ParthenonChain - Cross-Chain Bridges
// Bridges to major blockchains (Bitcoin, Ethereum, etc.)

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace layer2 {
namespace bridges {

/**
 * Supported blockchain networks
 */
enum class BlockchainNetwork { BITCOIN, ETHEREUM, BINANCE_SMART_CHAIN, POLYGON, AVALANCHE, SOLANA };

/**
 * Cross-chain transaction
 */
struct CrossChainTx {
    BlockchainNetwork source_chain;
    BlockchainNetwork destination_chain;
    std::vector<uint8_t> source_tx_hash;
    std::vector<uint8_t> destination_tx_hash;
    std::vector<uint8_t> source_address;
    std::vector<uint8_t> destination_address;
    uint64_t amount;
    std::string asset;
    uint64_t timestamp;
    std::vector<uint8_t> proof;
    bool finalized;

    CrossChainTx()
        : source_chain(BlockchainNetwork::BITCOIN),
          destination_chain(BlockchainNetwork::ETHEREUM),
          amount(0),
          timestamp(0),
          finalized(false) {}
};

/**
 * Bitcoin/Ethereum Bridge
 * Simplified unified bridge interface
 */
class CrossChainBridge {
  public:
    CrossChainBridge();
    ~CrossChainBridge();

    /**
     * Lock assets on source chain
     */
    bool LockAsset(BlockchainNetwork source_chain, const std::string& source_address,
                   uint64_t amount, const std::vector<uint8_t>& dest_address);

    /**
     * Unlock assets on destination chain
     */
    bool UnlockAsset(BlockchainNetwork dest_chain, const std::vector<uint8_t>& source_address,
                     uint64_t amount, const std::string& dest_address);

    /**
     * Verify cross-chain transaction
     */
    bool VerifyCrossChainTx(const CrossChainTx& tx);

    /**
     * Get wrapped token balance
     */
    uint64_t GetWrappedBalance(const std::vector<uint8_t>& address, BlockchainNetwork chain) const;

  private:
    std::map<std::pair<std::vector<uint8_t>, BlockchainNetwork>, uint64_t> balances_;
};

}  // namespace bridges
}  // namespace layer2
}  // namespace parthenon
