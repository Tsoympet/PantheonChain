// ParthenonChain - Node Infrastructure
// Blockchain synchronization and peer management

#pragma once

#include "primitives/block.h"
#include "chainstate/chainstate.h"
#include "chainstate/chain.h"
#include "p2p/protocol.h"
#include "mempool/mempool.h"
#include "storage/block_storage.h"
#include "storage/utxo_storage.h"
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace parthenon {
namespace node {

/**
 * Peer connection information
 */
struct PeerInfo {
    std::string address;
    uint16_t port;
    uint32_t version;
    uint64_t height;
    bool is_connected;
    uint64_t last_seen;
};

/**
 * Block synchronization status
 */
struct SyncStatus {
    bool is_syncing;
    uint32_t current_height;
    uint32_t target_height;
    uint32_t blocks_remaining;
    double progress_percent;
};

/**
 * Node manages blockchain state and peer connections
 */
class Node {
public:
    /**
     * Create a node
     * @param data_dir Directory for blockchain data
     * @param port P2P listening port
     */
    Node(const std::string& data_dir, uint16_t port = 8333);
    ~Node();
    
    /**
     * Start the node
     * - Initializes chainstate
     * - Starts P2P network
     * - Begins block synchronization
     */
    bool Start();
    
    /**
     * Stop the node gracefully
     */
    void Stop();
    
    /**
     * Check if node is running
     */
    bool IsRunning() const { return running_; }
    
    /**
     * Get current synchronization status
     */
    SyncStatus GetSyncStatus() const;
    
    /**
     * Get list of connected peers
     */
    std::vector<PeerInfo> GetPeers() const;
    
    /**
     * Add a peer to connect to
     * @param address Peer IP address
     * @param port Peer port
     */
    void AddPeer(const std::string& address, uint16_t port);
    
    /**
     * Process incoming block from peer
     * @param block Block to process
     * @param peer_id Peer that sent the block
     * @return true if block was accepted
     */
    bool ProcessBlock(const primitives::Block& block, const std::string& peer_id);
    
    /**
     * Submit a transaction to mempool
     * @param tx Transaction to submit
     * @return true if transaction was accepted
     */
    bool SubmitTransaction(const primitives::Transaction& tx);
    
    /**
     * Get current blockchain height
     */
    uint32_t GetHeight() const;
    
    /**
     * Get block by height
     */
    std::optional<primitives::Block> GetBlockByHeight(uint32_t height) const;
    
    /**
     * Get block by hash
     */
    std::optional<primitives::Block> GetBlockByHash(const std::array<uint8_t, 32>& hash) const;
    
    /**
     * Register callback for new blocks
     */
    void OnNewBlock(std::function<void(const primitives::Block&)> callback);
    
    /**
     * Register callback for new transactions
     */
    void OnNewTransaction(std::function<void(const primitives::Transaction&)> callback);
    
private:
    std::string data_dir_;
    uint16_t port_;
    bool running_;
    
    // Core components
    std::unique_ptr<chainstate::Chain> chain_;
    std::unique_ptr<mempool::Mempool> mempool_;
    
    // Peer management
    std::map<std::string, PeerInfo> peers_;
    
    // Synchronization state
    bool is_syncing_;
    uint32_t sync_target_height_;
    
    // Callbacks
    std::vector<std::function<void(const primitives::Block&)>> block_callbacks_;
    std::vector<std::function<void(const primitives::Transaction&)>> tx_callbacks_;
    
    // Storage backends
    std::unique_ptr<storage::BlockStorage> block_storage_;
    std::unique_ptr<storage::UTXOStorage> utxo_storage_;
    
    // Internal methods
    void SyncLoop();
    void RequestBlocks(const std::string& peer_id, uint32_t start_height, uint32_t count);
    bool ValidateAndApplyBlock(const primitives::Block& block);
    void BroadcastBlock(const primitives::Block& block);
    void BroadcastTransaction(const primitives::Transaction& tx);
};

} // namespace node
} // namespace parthenon
