// ParthenonChain - Wallet Module
// HD Wallet with UTXO management

#pragma once

#include "chainstate/utxo.h"
#include "crypto/schnorr.h"
#include "primitives/asset.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace wallet {

/**
 * Wallet address with associated key
 */
struct WalletAddress {
    std::vector<uint8_t> pubkey;  // 32-byte x-only public key
    std::string label;
    uint64_t index;  // HD derivation index
};

/**
 * UTXO owned by wallet
 */
struct WalletUTXO {
    primitives::OutPoint outpoint;
    primitives::TxOutput output;
    uint32_t height;  // Block height of UTXO
    bool is_spent;
};

/**
 * Wallet class implementing HD wallet functionality
 *
 * Features:
 * - BIP-32 hierarchical deterministic key derivation
 * - UTXO tracking and management
 * - Transaction construction and signing
 * - Multi-asset balance tracking
 */
class Wallet {
  public:
    /**
     * Create new wallet from seed
     * @param seed 256-bit seed for HD wallet
     */
    explicit Wallet(const std::array<uint8_t, 32>& seed);

    /**
     * Generate a new receiving address
     * @param label Optional label for the address
     * @return New wallet address
     */
    WalletAddress GenerateAddress(const std::string& label = "");

    /**
     * Get all wallet addresses
     */
    std::vector<WalletAddress> GetAddresses() const;

    /**
     * Get wallet balance for an asset
     * @param asset Asset ID
     * @return Total balance
     */
    uint64_t GetBalance(primitives::AssetID asset) const;

    /**
     * Get all wallet balances
     */
    std::map<primitives::AssetID, uint64_t> GetBalances() const;

    /**
     * List all UTXOs owned by wallet
     * @param include_spent Include spent UTXOs
     */
    std::vector<WalletUTXO> ListUTXOs(bool include_spent = false) const;

    /**
     * Create a transaction
     * @param outputs Transaction outputs
     * @param asset Asset to spend
     * @param fee_amount Fee to pay
     * @return Signed transaction or nullopt if insufficient funds
     */
    std::optional<primitives::Transaction>
    CreateTransaction(const std::vector<primitives::TxOutput>& outputs, primitives::AssetID asset,
                      uint64_t fee_amount);

    /**
     * Add UTXO to wallet tracking
     * @param outpoint Output point
     * @param output Transaction output
     * @param height Block height
     */
    void AddUTXO(const primitives::OutPoint& outpoint, const primitives::TxOutput& output,
                 uint32_t height);

    /**
     * Mark UTXO as spent
     * @param outpoint Output point to mark as spent
     */
    void MarkSpent(const primitives::OutPoint& outpoint);

    /**
     * Sync wallet with chainstate
     * @param utxo_set Current UTXO set
     */
    void SyncWithChain(const chainstate::UTXOSet& utxo_set);

    /**
     * Process a new block and update wallet state
     * @param block Block to process
     * @param height Block height
     */
    void ProcessBlock(const primitives::Block& block, uint32_t height);

    /**
     * Check if a pubkey belongs to this wallet
     * @param pubkey Public key to check
     * @return true if pubkey is owned by wallet
     */
    bool IsOurPubkey(const std::vector<uint8_t>& pubkey) const;

  private:
    std::array<uint8_t, 32> seed_;
    std::vector<WalletAddress> addresses_;
    std::vector<WalletUTXO> utxos_;
    std::map<uint64_t, crypto::Schnorr::PrivateKey> keys_;  // index -> private key
    uint64_t next_index_;

    // Derive private key at index
    crypto::Schnorr::PrivateKey DeriveKey(uint64_t index);

    // Select UTXOs for spending
    std::vector<WalletUTXO> SelectCoins(primitives::AssetID asset, uint64_t amount);
};

}  // namespace wallet
}  // namespace parthenon
