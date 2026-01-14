#ifndef PARTHENON_TOOLS_BLOCK_EXPLORER_EXPLORER_H
#define PARTHENON_TOOLS_BLOCK_EXPLORER_EXPLORER_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <optional>

namespace parthenon {
namespace tools {
namespace explorer {

/**
 * Block Explorer Data Structures
 */

struct BlockInfo {
    uint64_t height;
    std::string hash;
    std::string prev_hash;
    uint64_t timestamp;
    std::string miner;
    uint64_t transaction_count;
    uint64_t size;
    uint64_t difficulty;
    std::string merkle_root;
};

struct TransactionInfo {
    std::string txid;
    uint64_t block_height;
    std::string block_hash;
    uint64_t timestamp;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    uint64_t fee;
    std::string status;
};

struct AddressInfo {
    std::string address;
    uint64_t balance;
    uint64_t tx_count;
    uint64_t received_total;
    uint64_t sent_total;
    std::vector<TransactionInfo> recent_txs;
};

/**
 * Block Explorer API
 * Web-based blockchain explorer interface
 */
class BlockExplorerAPI {
public:
    BlockExplorerAPI();
    ~BlockExplorerAPI();
    
    /**
     * Get block by height or hash
     */
    std::optional<BlockInfo> GetBlock(const std::string& identifier);
    
    /**
     * Get latest blocks
     */
    std::vector<BlockInfo> GetLatestBlocks(size_t count = 10);
    
    /**
     * Get transaction by ID
     */
    std::optional<TransactionInfo> GetTransaction(const std::string& txid);
    
    /**
     * Get address information
     */
    std::optional<AddressInfo> GetAddress(const std::string& address);
    
    /**
     * Search by block, transaction, or address
     */
    std::string Search(const std::string& query);
    
    /**
     * Get blockchain statistics
     */
    struct ChainStats {
        uint64_t height;
        uint64_t total_transactions;
        uint64_t total_addresses;
        uint64_t avg_block_time;
        uint64_t total_supply;
        double hashrate;
    };
    
    ChainStats GetStatistics();
    
    /**
     * Get mempool information
     */
    struct MempoolInfo {
        uint64_t tx_count;
        uint64_t total_size;
        uint64_t avg_fee;
        std::vector<TransactionInfo> top_fee_txs;
    };
    
    MempoolInfo GetMempool();
    
private:
    std::map<std::string, BlockInfo> blocks_;
    std::map<std::string, TransactionInfo> transactions_;
    std::map<std::string, AddressInfo> addresses_;
};

/**
 * Explorer Web Server
 * HTTP server for block explorer
 */
class ExplorerWebServer {
public:
    ExplorerWebServer(uint16_t port = 8080);
    ~ExplorerWebServer();
    
    /**
     * Start server
     */
    bool Start();
    
    /**
     * Stop server
     */
    void Stop();
    
    /**
     * Set API handler
     */
    void SetAPI(BlockExplorerAPI* api) { api_ = api; }
    
    /**
     * Get server port
     */
    uint16_t GetPort() const { return port_; }
    
private:
    uint16_t port_;
    bool running_;
    BlockExplorerAPI* api_;
    
    void HandleRequest(const std::string& path);
};

/**
 * Chart Data Provider
 * Provides data for charts and graphs
 */
class ChartDataProvider {
public:
    struct DataPoint {
        uint64_t timestamp;
        double value;
    };
    
    /**
     * Get price history
     */
    std::vector<DataPoint> GetPriceHistory(uint64_t days = 30);
    
    /**
     * Get transaction volume history
     */
    std::vector<DataPoint> GetTxVolumeHistory(uint64_t days = 30);
    
    /**
     * Get difficulty history
     */
    std::vector<DataPoint> GetDifficultyHistory(uint64_t days = 30);
    
    /**
     * Get hashrate history
     */
    std::vector<DataPoint> GetHashrateHistory(uint64_t days = 30);
};

} // namespace explorer
} // namespace tools
} // namespace parthenon

#endif // PARTHENON_TOOLS_BLOCK_EXPLORER_EXPLORER_H
