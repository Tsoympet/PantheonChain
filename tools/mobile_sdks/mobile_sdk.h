// ParthenonChain Mobile SDK - Core Library
// Cross-platform C++ library for iOS and Android

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>

namespace parthenon {
namespace mobile {

/**
 * Network configuration
 */
struct NetworkConfig {
    std::string endpoint;          // RPC endpoint URL
    std::string network_id;        // "mainnet", "testnet", etc.
    uint32_t chain_id;
    uint32_t timeout_ms;           // Request timeout in milliseconds
    
    NetworkConfig() : chain_id(0), timeout_ms(30000) {}
};

/**
 * Transaction data
 */
struct Transaction {
    std::string from;
    std::string to;
    uint64_t amount;
    std::string asset;             // "TALN", "DRM", or "OBL"
    std::string memo;
    uint64_t fee;
    std::vector<uint8_t> signature;
    
    Transaction() : amount(0), asset("TALN"), fee(0) {}
};

/**
 * Balance information
 */
struct Balance {
    uint64_t taln;
    uint64_t drm;
    uint64_t obl;
    
    Balance() : taln(0), drm(0), obl(0) {}
};

/**
 * Transaction history item
 */
struct TransactionHistory {
    std::string txid;
    std::string from;
    std::string to;
    uint64_t amount;
    std::string asset;
    uint64_t timestamp;
    std::string status;            // "confirmed", "pending", "failed"
    uint32_t confirmations;
    
    TransactionHistory() : amount(0), timestamp(0), confirmations(0) {}
};

/**
 * Smart contract call data
 */
struct ContractCall {
    std::string contract_address;
    std::string function_name;
    std::vector<std::string> parameters;
    uint64_t gas_limit;
    
    ContractCall() : gas_limit(0) {}
};

/**
 * Wallet management
 */
class Wallet {
public:
    /**
     * Generate new wallet
     */
    static std::unique_ptr<Wallet> Generate();
    
    /**
     * Import wallet from mnemonic
     */
    static std::unique_ptr<Wallet> FromMnemonic(const std::string& mnemonic);
    
    /**
     * Import wallet from private key
     */
    static std::unique_ptr<Wallet> FromPrivateKey(const std::vector<uint8_t>& private_key);
    
    /**
     * Get wallet address
     */
    std::string GetAddress() const;
    
    /**
     * Get public key
     */
    std::vector<uint8_t> GetPublicKey() const;
    
    /**
     * Sign transaction
     */
    std::vector<uint8_t> SignTransaction(const Transaction& tx);
    
    /**
     * Sign message
     */
    std::vector<uint8_t> SignMessage(const std::string& message);
    
    /**
     * Export mnemonic
     */
    std::string ExportMnemonic() const;
    
    /**
     * Export private key
     */
    std::vector<uint8_t> ExportPrivateKey() const;
    
private:
    Wallet();
    
    std::vector<uint8_t> private_key_;
    std::vector<uint8_t> public_key_;
    std::string address_;
    std::string mnemonic_;
};

/**
 * Mobile client for ParthenonChain
 */
class MobileClient {
public:
    explicit MobileClient(const NetworkConfig& config);
    ~MobileClient();
    
    /**
     * Get balance for address
     */
    using BalanceCallback = std::function<void(std::optional<Balance>, std::optional<std::string>)>;
    void GetBalance(const std::string& address, BalanceCallback callback);
    
    /**
     * Send transaction
     */
    using TransactionCallback = std::function<void(std::optional<std::string>, std::optional<std::string>)>;
    void SendTransaction(const Transaction& tx, TransactionCallback callback);
    
    /**
     * Get transaction history
     */
    using HistoryCallback = std::function<void(std::vector<TransactionHistory>, std::optional<std::string>)>;
    void GetTransactionHistory(const std::string& address, uint32_t limit, HistoryCallback callback);
    
    /**
     * Get transaction by hash
     */
    using TxInfoCallback = std::function<void(std::optional<TransactionHistory>, std::optional<std::string>)>;
    void GetTransaction(const std::string& txid, TxInfoCallback callback);
    
    /**
     * Call smart contract (read-only)
     */
    using ContractCallCallback = std::function<void(std::optional<std::string>, std::optional<std::string>)>;
    void CallContract(const ContractCall& call, ContractCallCallback callback);
    
    /**
     * Deploy smart contract
     */
    void DeployContract(const std::vector<uint8_t>& bytecode, TransactionCallback callback);
    
    /**
     * Subscribe to new blocks
     */
    using BlockCallback = std::function<void(uint64_t block_height, const std::string& block_hash)>;
    void SubscribeToBlocks(BlockCallback callback);
    
    /**
     * Subscribe to address transactions
     */
    using AddressTxCallback = std::function<void(const TransactionHistory& tx)>;
    void SubscribeToAddress(const std::string& address, AddressTxCallback callback);
    
    /**
     * Estimate gas for transaction
     */
    using GasEstimateCallback = std::function<void(std::optional<uint64_t>, std::optional<std::string>)>;
    void EstimateGas(const Transaction& tx, GasEstimateCallback callback);
    
    /**
     * Get current network status
     */
    struct NetworkStatus {
        uint64_t block_height;
        uint64_t peer_count;
        bool syncing;
        std::string network_id;
    };
    using NetworkStatusCallback = std::function<void(std::optional<NetworkStatus>, std::optional<std::string>)>;
    void GetNetworkStatus(NetworkStatusCallback callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * QR Code generator/scanner
 */
class QRCodeHelper {
public:
    /**
     * Generate payment QR code
     * Format: pantheon:<address>?amount=<amount>&asset=<asset>&memo=<memo>
     */
    static std::string GeneratePaymentURI(
        const std::string& address,
        uint64_t amount,
        const std::string& asset = "TALN",
        const std::string& memo = ""
    );
    
    /**
     * Parse payment URI from QR code
     */
    struct PaymentRequest {
        std::string address;
        uint64_t amount;
        std::string asset;
        std::string memo;
    };
    
    static std::optional<PaymentRequest> ParsePaymentURI(const std::string& uri);
};

/**
 * Local storage for mobile app
 */
class SecureStorage {
public:
    /**
     * Store encrypted data
     */
    static bool Store(const std::string& key, const std::vector<uint8_t>& data);
    
    /**
     * Retrieve encrypted data
     */
    static std::optional<std::vector<uint8_t>> Retrieve(const std::string& key);
    
    /**
     * Delete data
     */
    static bool Delete(const std::string& key);
    
    /**
     * Check if key exists
     */
    static bool Exists(const std::string& key);
};

} // namespace mobile
} // namespace parthenon
