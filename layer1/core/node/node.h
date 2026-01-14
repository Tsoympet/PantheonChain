// ParthenonChain - Node Infrastructure
// Blockchain synchronization and peer management

#pragma once

#include "chainstate/chain.h"
#include "chainstate/chainstate.h"
#include "mempool/mempool.h"
#include "p2p/network_manager.h"
#include "p2p/protocol.h"
#include "primitives/block.h"

#include "mining/miner.h"
#include "storage/block_storage.h"
#include "storage/utxo_storage.h"
#include "wallet/wallet.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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

    /**
     * Start mining blocks
     * @param coinbase_pubkey Public key for receiving mining rewards
     * @param num_threads Number of mining threads (0 = auto-detect)
     */
    void StartMining(const std::vector<uint8_t>& coinbase_pubkey, size_t num_threads = 0);

    /**
     * Stop mining
     */
    void StopMining();

    /**
     * Check if mining is active
     */
    bool IsMining() const { return is_mining_; }

    /**
     * Get mining statistics
     */
    struct MiningStats {
        bool is_mining;
        uint64_t hashrate;
        uint32_t blocks_mined;
        uint32_t current_height;
        uint64_t total_hashes;
    };
    MiningStats GetMiningStats() const;

    /**
     * Attach a wallet for UTXO synchronization
     * Wallet will be automatically updated when blocks are processed
     * @param wallet Shared pointer to wallet instance
     */
    void AttachWallet(std::shared_ptr<wallet::Wallet> wallet);

    /**
     * Detach wallet from node
     */
    void DetachWallet();

    /**
     * Get attached wallet
     * @return Shared pointer to wallet, or nullptr if no wallet attached
     */
    std::shared_ptr<wallet::Wallet> GetWallet() const { return wallet_; }

    /**
     * Sync wallet with current blockchain state
     * Processes all blocks from genesis to current tip
     */
    void SyncWalletWithChain();

  private:
    std::string data_dir_;
    uint16_t port_;
    bool running_;

    // Core components
    std::unique_ptr<chainstate::Chain> chain_;
    std::unique_ptr<mempool::Mempool> mempool_;

    // Peer management
    std::unique_ptr<p2p::NetworkManager> network_;
    std::map<std::string, PeerInfo> peers_;

    // Synchronization state
    bool is_syncing_;
    uint32_t sync_target_height_;
    std::thread sync_thread_;

    // Callbacks
    std::vector<std::function<void(const primitives::Block&)>> block_callbacks_;
    std::vector<std::function<void(const primitives::Transaction&)>> tx_callbacks_;

    // Storage backends
    std::unique_ptr<storage::BlockStorage> block_storage_;
    std::unique_ptr<storage::UTXOStorage> utxo_storage_;

    // Mining
    std::unique_ptr<mining::Miner> miner_;
    std::atomic<bool> is_mining_;
    std::vector<std::thread> mining_threads_;
    std::atomic<uint64_t> total_hashes_;
    std::atomic<uint32_t> blocks_mined_;
    std::vector<uint8_t> coinbase_pubkey_;

    // Wallet for UTXO tracking
    std::shared_ptr<wallet::Wallet> wallet_;
    std::mutex wallet_mutex_;

    // Internal methods
    void SyncLoop();
    void MiningLoop(size_t thread_id);
    void RequestBlocks(const std::string& peer_id, uint32_t start_height, uint32_t count);
    bool ValidateAndApplyBlock(const primitives::Block& block);
    void BroadcastBlock(const primitives::Block& block);
    void BroadcastTransaction(const primitives::Transaction& tx);
    void HandleNewPeer(const std::string& peer_id);
    void HandleBlockReceived(const std::string& peer_id, const primitives::Block& block);
    void HandleTxReceived(const std::string& peer_id, const primitives::Transaction& tx);
    void HandleInvReceived(const std::string& peer_id, const p2p::InvMessage& inv);
    void HandleGetDataReceived(const std::string& peer_id, const p2p::GetDataMessage& msg);
};

}  // namespace node
}  // namespace parthenon
