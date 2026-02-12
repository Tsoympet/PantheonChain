// ParthenonChain - Node Infrastructure Implementation

#include "node.h"

#include "validation/validation.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace parthenon {
namespace node {

Node::Node(const std::string& data_dir, uint16_t port)
    : data_dir_(data_dir),
      port_(port),
      running_(false),
      is_syncing_(false),
      sync_target_height_(0),
      is_mining_(false),
      total_hashes_(0),
      blocks_mined_(0) {
    // Initialize components
    chain_ = std::make_unique<chainstate::Chain>();
    mempool_ = std::make_unique<mempool::Mempool>();
    block_storage_ = std::make_unique<storage::BlockStorage>();
    utxo_storage_ = std::make_unique<storage::UTXOStorage>();

    // Initialize P2P network manager
    network_ = std::make_unique<p2p::NetworkManager>(port, p2p::NetworkMagic::MAINNET);
}

Node::~Node() {
    Stop();
}

bool Node::Start() {
    if (running_) {
        return false;
    }

    std::cout << "Starting ParthenonChain node on port " << port_ << std::endl;

    // Open block storage database
    std::string block_db_path = data_dir_ + "/blocks";
    if (!block_storage_->Open(block_db_path)) {
        std::cerr << "Failed to open block storage database" << std::endl;
        return false;
    }
    std::cout << "Opened block storage at " << block_db_path << std::endl;

    // Open UTXO storage database
    std::string utxo_db_path = data_dir_ + "/utxo";
    if (!utxo_storage_->Open(utxo_db_path)) {
        std::cerr << "Failed to open UTXO storage database" << std::endl;
        block_storage_->Close();
        return false;
    }
    std::cout << "Opened UTXO storage at " << utxo_db_path << std::endl;

    // Load blockchain height from storage
    uint32_t stored_height = block_storage_->GetHeight();
    std::cout << "Loaded blockchain height: " << stored_height << std::endl;

    // Set up network callbacks
    network_->SetOnNewPeer([this](const std::string& peer_id) { HandleNewPeer(peer_id); });

    network_->SetOnBlock([this](const std::string& peer_id, const primitives::Block& block) {
        HandleBlockReceived(peer_id, block);
    });

    network_->SetOnTransaction(
        [this](const std::string& peer_id, const primitives::Transaction& tx) {
            HandleTxReceived(peer_id, tx);
        });

    network_->SetOnInv([this](const std::string& peer_id, const p2p::InvMessage& inv) {
        HandleInvReceived(peer_id, inv);
    });

    network_->SetOnGetData([this](const std::string& peer_id, const p2p::GetDataMessage& msg) {
        HandleGetDataReceived(peer_id, msg);
    });

    // Start network manager
    if (!network_->Start()) {
        std::cerr << "Failed to start P2P network" << std::endl;
        block_storage_->Close();
        utxo_storage_->Close();
        return false;
    }
    std::cout << "P2P network started on port " << port_ << std::endl;

    // Add DNS seeds for peer discovery
    network_->AddDNSSeed("seed.pantheonchain.io", 8333);
    network_->AddDNSSeed("seed2.pantheonchain.io", 8333);

    // Query DNS seeds for initial peers
    std::cout << "Querying DNS seeds for peers..." << std::endl;
    network_->QueryDNSSeeds();

    running_ = true;
    is_syncing_ = true;

    // Start sync loop in background thread
    std::cout << "Starting background sync thread" << std::endl;
    sync_thread_ = std::thread(&Node::SyncLoop, this);

    std::cout << "Node started successfully" << std::endl;
    return true;
}

void Node::Stop() {
    if (!running_) {
        return;
    }

    std::cout << "Stopping node..." << std::endl;

    // Stop mining first
    if (is_mining_) {
        StopMining();
    }

    // Stop sync thread
    is_syncing_ = false;
    if (sync_thread_.joinable()) {
        sync_thread_.join();
    }

    // Stop P2P network
    std::cout << "Stopping P2P network..." << std::endl;
    network_->Stop();

    // Save UTXO set to disk
    std::cout << "Saving UTXO set to disk..." << std::endl;
    auto& utxo_set = chain_->GetUTXOSet();
    utxo_storage_->SaveUTXOSet(utxo_set);

    // Close storage databases
    std::cout << "Closing storage databases..." << std::endl;
    block_storage_->Close();
    utxo_storage_->Close();

    running_ = false;

    std::cout << "Node stopped" << std::endl;
}

SyncStatus Node::GetSyncStatus() const {
    SyncStatus status;
    status.is_syncing = is_syncing_;
    status.current_height = GetHeight();
    status.target_height = sync_target_height_;

    if (sync_target_height_ > 0) {
        status.blocks_remaining = sync_target_height_ - status.current_height;
        status.progress_percent = (static_cast<double>(status.current_height) /
                                   static_cast<double>(sync_target_height_)) *
                                  100.0;
    } else {
        status.blocks_remaining = 0;
        status.progress_percent = 100.0;
    }

    return status;
}

std::vector<PeerInfo> Node::GetPeers() const {
    std::vector<PeerInfo> peer_list;
    for (const auto& [id, info] : peers_) {
        peer_list.push_back(info);
    }
    return peer_list;
}

void Node::AddPeer(const std::string& address, uint16_t port) {
    std::string peer_id = address + ":" + std::to_string(port);

    if (peers_.find(peer_id) != peers_.end()) {
        return;  // Already have this peer
    }

    PeerInfo info;
    info.address = address;
    info.port = port;
    info.version = 1;
    info.height = 0;
    info.is_connected = false;
    info.last_seen = 0;

    peers_[peer_id] = info;

    // Initiate connection to peer via network manager
    if (network_ && running_) {
        network_->AddPeer(address, port);
        std::cout << "Connecting to peer: " << peer_id << std::endl;
    } else {
        std::cout << "Added peer (will connect when node starts): " << peer_id << std::endl;
    }
}

bool Node::ProcessBlock(const primitives::Block& block, const std::string& peer_id) {
    // Validate block
    if (!ValidateAndApplyBlock(block)) {
        std::cout << "Rejected invalid block from peer: " << peer_id << std::endl;
        return false;
    }

    // Update sync status
    uint32_t block_height = GetHeight();
    if (block_height >= sync_target_height_) {
        is_syncing_ = false;
    }

    // Update wallet if attached
    {
        std::lock_guard<std::mutex> lock(wallet_mutex_);
        if (wallet_) {
            wallet_->ProcessBlock(block, block_height);
        }
    }

    // Notify callbacks
    for (const auto& callback : block_callbacks_) {
        callback(block);
    }

    // Broadcast to other peers
    BroadcastBlock(block);

    return true;
}

bool Node::SubmitTransaction(const primitives::Transaction& tx) {
    // Validate transaction
    auto error = validation::TransactionValidator::ValidateStructure(tx);
    if (error) {
        std::cout << "Rejected invalid transaction: " << error->message << std::endl;
        return false;
    }

    error = validation::TransactionValidator::ValidateAgainstUTXO(tx, chain_->GetUTXOSet(),
                                                                  GetHeight());
    if (error) {
        std::cout << "Transaction validation failed: " << error->message << std::endl;
        return false;
    }

    error = validation::TransactionValidator::ValidateSignatures(tx, chain_->GetUTXOSet());
    if (error) {
        std::cout << "Invalid transaction signature: " << error->message << std::endl;
        return false;
    }

    // Add to mempool
    if (!mempool_->AddTransaction(tx, chain_->GetUTXOSet(), GetHeight())) {
        std::cout << "Transaction already in mempool" << std::endl;
        return false;
    }

    // Notify callbacks
    for (const auto& callback : tx_callbacks_) {
        callback(tx);
    }

    // Broadcast to peers
    BroadcastTransaction(tx);

    return true;
}

uint32_t Node::GetHeight() const {
    return static_cast<uint32_t>(chain_->GetHeight());
}

std::optional<primitives::Block> Node::GetBlockByHeight(uint32_t height) const {
    if (block_storage_ && block_storage_->IsOpen()) {
        return block_storage_->GetBlockByHeight(height);
    }
    return std::nullopt;
}

std::optional<primitives::Block> Node::GetBlockByHash(const std::array<uint8_t, 32>& hash) const {
    if (block_storage_ && block_storage_->IsOpen()) {
        return block_storage_->GetBlockByHash(hash);
    }
    return std::nullopt;
}

void Node::OnNewBlock(std::function<void(const primitives::Block&)> callback) {
    block_callbacks_.push_back(callback);
}

void Node::OnNewTransaction(std::function<void(const primitives::Transaction&)> callback) {
    tx_callbacks_.push_back(callback);
}

void Node::SyncLoop() {
    std::cout << "Starting sync loop..." << std::endl;

    while (is_syncing_ && running_) {
        uint32_t current_height = GetHeight();

        // Get connected peers
        auto peers = GetPeers();

        if (peers.empty()) {
            std::cout << "No peers connected, waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // If we haven't reached the target, request more blocks
        if (current_height < sync_target_height_) {
            std::cout << "Syncing: " << current_height << "/" << sync_target_height_ << std::endl;

            // Request blocks from first available peer
            if (!peers.empty()) {
                std::string peer_id = peers[0].address + ":" + std::to_string(peers[0].port);
                RequestBlocks(peer_id, current_height + 1, 500);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            // Caught up or no target set
            if (sync_target_height_ > 0) {
                is_syncing_ = false;
                std::cout << "Sync complete at height " << current_height << std::endl;
            } else {
                // No target yet, query peers for their heights
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    std::cout << "Sync loop exited" << std::endl;
}

void Node::RequestBlocks(const std::string& peer_id, uint32_t start_height, uint32_t count) {
    std::cout << "Requesting " << count << " blocks starting at " << start_height << " from peer "
              << peer_id << std::endl;

    if (network_) {
        network_->RequestBlocks(peer_id, start_height, count);
    }
}

bool Node::ValidateAndApplyBlock(const primitives::Block& block) {
    if (!chain_state_.ValidateBlock(block)) {
        std::cerr << "Block failed chain state validation" << std::endl;
        return false;
    }

    // Validate all transactions
    for (const auto& tx : block.transactions) {
        // Skip validation for coinbase (first transaction)
        if (&tx == &block.transactions[0]) {
            continue;
        }

        // Validate structure
        auto error = validation::TransactionValidator::ValidateStructure(tx);
        if (error) {
            return false;
        }

        // Validate against UTXO
        error = validation::TransactionValidator::ValidateAgainstUTXO(tx, chain_->GetUTXOSet(),
                                                                      GetHeight());
        if (error) {
            return false;
        }

        // Validate signatures
        error = validation::TransactionValidator::ValidateSignatures(tx, chain_->GetUTXOSet());
        if (error) {
            return false;
        }
    }

    // Apply block to chain
    chainstate::BlockUndo undo;
    if (!chain_->ConnectBlock(block, undo)) {
        return false;
    }

    if (!chain_state_.ApplyBlock(block)) {
        std::cerr << "Warning: failed to update mining chain state; mining height may be stale"
                  << std::endl;
    }

    // Store block to disk
    uint32_t height = chain_->GetHeight();  // Get height from chain state
    if (block_storage_ && block_storage_->IsOpen()) {
        block_storage_->StoreBlock(block, height);
        auto block_hash = block.GetHash();
        block_storage_->UpdateChainTip(height, block_hash);
    }

    // Update UTXO storage
    if (utxo_storage_ && utxo_storage_->IsOpen()) {
        utxo_storage_->SaveUTXOSet(chain_->GetUTXOSet());
    }

    // Remove transactions from mempool
    for (const auto& tx : block.transactions) {
        mempool_->RemoveTransaction(tx.GetTxID());
    }

    std::cout << "Block " << height << " validated, applied, and stored" << std::endl;
    return true;
}

void Node::BroadcastBlock(const primitives::Block& block) {
    if (network_) {
        network_->BroadcastBlock(block);
    }
}

void Node::BroadcastTransaction(const primitives::Transaction& tx) {
    if (network_) {
        network_->BroadcastTransaction(tx);
    }
}

void Node::HandleNewPeer(const std::string& peer_id) {
    std::cout << "New peer connected: " << peer_id << std::endl;
    auto colon_pos = peer_id.find(':');
    std::string address = peer_id;
    uint16_t port = 0;
    if (colon_pos != std::string::npos) {
        address = peer_id.substr(0, colon_pos);
        auto port_str = peer_id.substr(colon_pos + 1);
        bool port_valid = false;
        if (!port_str.empty() && port_str.size() <= 5) {
            try {
                auto parsed_port = std::stoul(port_str);
                if (parsed_port <= 65535) {
                    port = static_cast<uint16_t>(parsed_port);
                    port_valid = true;
                }
            } catch (...) {
                port_valid = false;
            }
        }

        if (!port_valid) {
            std::cerr << "Invalid peer port for peer: " << peer_id << std::endl;
            port = 0;
        }
    }

    auto now = std::chrono::system_clock::now();
    auto last_seen = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

    auto it = peers_.find(peer_id);
    if (it == peers_.end()) {
        PeerInfo info;
        info.address = address;
        info.port = port;
        info.version = 1;
        info.height = 0;
        info.is_connected = true;
        info.last_seen = last_seen;
        peers_[peer_id] = info;
    } else {
        it->second.is_connected = true;
        it->second.last_seen = last_seen;
    }
    // Update sync target based on peer height
    // In production: Query peer for their best block height
}

void Node::HandleBlockReceived(const std::string& peer_id, const primitives::Block& block) {
    std::cout << "Received block from " << peer_id << std::endl;

    // Process the block
    ProcessBlock(block, peer_id);
}

void Node::HandleTxReceived(const std::string& peer_id, const primitives::Transaction& tx) {
    std::cout << "Received transaction from " << peer_id << std::endl;

    // Submit to mempool
    SubmitTransaction(tx);
}

void Node::HandleInvReceived(const std::string& peer_id, const p2p::InvMessage& inv) {
    std::cout << "Received inventory from " << peer_id << ": " << inv.inventory.size() << " items"
              << std::endl;

    // Request data for items we don't have
    p2p::GetDataMessage getdata;
    for (const auto& item : inv.inventory) {
        // Check if we already have this item
        bool have_item = false;

        if (item.type == p2p::InvType::MSG_BLOCK) {
            have_item = block_storage_ && block_storage_->GetBlockByHash(item.hash).has_value();
        } else if (item.type == p2p::InvType::MSG_TX) {
            // Check mempool or recent blocks
            // have_item = mempool_->HasTransaction(item.hash);
        }

        if (!have_item) {
            getdata.inventory.push_back(item);
        }
    }

    // Request items we don't have
    if (!getdata.inventory.empty() && network_) {
        // Send getdata to the peer
        // network_->SendGetData(peer_id, getdata);
    }
}

void Node::HandleGetDataReceived(const std::string& peer_id, const p2p::GetDataMessage& msg) {
    std::cout << "Received getdata from " << peer_id << ": " << msg.inventory.size() << " items"
              << std::endl;

    if (!network_) {
        return;
    }

    // Respond with requested items
    for (const auto& item : msg.inventory) {
        if (item.type == p2p::InvType::MSG_BLOCK) {
            if (block_storage_) {
                auto block = block_storage_->GetBlockByHash(item.hash);
                if (block) {
                    // Send block to peer
                    // network_->SendBlock(peer_id, *block);
                }
            }
        } else if (item.type == p2p::InvType::MSG_TX) {
            // Get transaction from mempool
            // if (mempool_->HasTransaction(item.hash)) {
            //     auto tx = mempool_->GetTransaction(item.hash);
            //     network_->SendTx(peer_id, tx);
            // }
        }
    }
}

// Mining functions
void Node::StartMining(const std::vector<uint8_t>& coinbase_pubkey, size_t num_threads) {
    if (is_mining_) {
        std::cout << "Mining already active" << std::endl;
        return;
    }

    if (!chain_) {
        std::cerr << "Cannot start mining: chain not initialized" << std::endl;
        return;
    }

    coinbase_pubkey_ = coinbase_pubkey;

    // Auto-detect thread count
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
            num_threads = 1;
    }

    miner_ = std::make_unique<mining::Miner>(chain_state_, coinbase_pubkey_);

    is_mining_ = true;
    total_hashes_ = 0;

    std::cout << "Starting mining with " << num_threads << " threads" << std::endl;

    // Start mining threads
    for (size_t i = 0; i < num_threads; ++i) {
        mining_threads_.emplace_back(&Node::MiningLoop, this, i);
    }
}

void Node::StopMining() {
    if (!is_mining_) {
        return;
    }

    std::cout << "Stopping mining..." << std::endl;
    is_mining_ = false;

    // Wait for mining threads to finish
    for (auto& thread : mining_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    mining_threads_.clear();
    miner_.reset();

    std::cout << "Mining stopped. Total blocks mined: " << blocks_mined_.load() << std::endl;
}

void Node::MiningLoop(size_t thread_id) {
    std::cout << "Mining thread " << thread_id << " started" << std::endl;

    const uint64_t ITERATIONS_PER_ROUND = 100000;  // Check for new blocks periodically

    while (is_mining_) {
        // Create block template
        auto template_opt = miner_->CreateBlockTemplate(1000);
        if (!template_opt) {
            // Failed to create template, wait and retry
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        auto& block_template = *template_opt;

        // Mine the block
        auto block_opt = miner_->MineBlock(block_template, ITERATIONS_PER_ROUND);
        total_hashes_ += ITERATIONS_PER_ROUND;

        if (block_opt) {
            // Successfully mined a block!
            auto& block = *block_opt;

            std::cout << "Thread " << thread_id << " mined block at height "
                      << block_template.height << std::endl;

            // Validate and apply the block
            if (ValidateAndApplyBlock(block)) {
                blocks_mined_++;

                // Broadcast to network
                BroadcastBlock(block);

                // Trigger callbacks
                for (const auto& callback : block_callbacks_) {
                    callback(block);
                }

                std::cout << "Block accepted! Total blocks mined: " << blocks_mined_.load()
                          << std::endl;
            } else {
                std::cerr << "Mined block failed validation!" << std::endl;
            }

            // Short pause before mining next block
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "Mining thread " << thread_id << " stopped" << std::endl;
}

Node::MiningStats Node::GetMiningStats() const {
    MiningStats stats;
    stats.is_mining = is_mining_;
    stats.blocks_mined = blocks_mined_;
    stats.total_hashes = total_hashes_;
    stats.current_height = GetHeight();

    // Calculate hashrate (approximate)
    if (miner_) {
        auto status = miner_->GetStatus();
        stats.hashrate = status.hashrate;
    } else {
        stats.hashrate = 0;
    }

    return stats;
}

void Node::AttachWallet(std::shared_ptr<wallet::Wallet> wallet) {
    std::lock_guard<std::mutex> lock(wallet_mutex_);
    wallet_ = wallet;
    std::cout << "Wallet attached to node" << std::endl;

    // Sync wallet with current chain state if node is running
    if (running_ && wallet_) {
        std::cout << "Syncing wallet with blockchain..." << std::endl;
        SyncWalletWithChain();
    }
}

void Node::DetachWallet() {
    std::lock_guard<std::mutex> lock(wallet_mutex_);
    wallet_ = nullptr;
    std::cout << "Wallet detached from node" << std::endl;
}

void Node::SyncWalletWithChain() {
    std::lock_guard<std::mutex> lock(wallet_mutex_);

    if (!wallet_) {
        std::cout << "No wallet attached" << std::endl;
        return;
    }

    std::cout << "Syncing wallet with chain..." << std::endl;

    uint32_t current_height = GetHeight();
    std::cout << "Processing " << current_height + 1 << " blocks..." << std::endl;

    // Process all blocks from genesis to current height
    for (uint32_t height = 0; height <= current_height; height++) {
        auto block_opt = GetBlockByHeight(height);
        if (block_opt) {
            wallet_->ProcessBlock(*block_opt, height);
        }

        // Show progress every 100 blocks
        if (height % 100 == 0 && height > 0) {
            std::cout << "  Processed " << height << " / " << current_height + 1 << " blocks"
                      << std::endl;
        }
    }

    std::cout << "Wallet sync complete!" << std::endl;

    // Display wallet balances
    auto balances = wallet_->GetBalances();
    std::cout << "Wallet balances:" << std::endl;
    std::cout << "  TALANTON: "
              << static_cast<double>(balances[primitives::AssetID::TALANTON]) / 100000000.0
              << std::endl;
    std::cout << "  DRACHMA:  "
              << static_cast<double>(balances[primitives::AssetID::DRACHMA]) / 100000000.0
              << std::endl;
    std::cout << "  OBOLOS:   "
              << static_cast<double>(balances[primitives::AssetID::OBOLOS]) / 100000000.0
              << std::endl;
}

}  // namespace node
}  // namespace parthenon
