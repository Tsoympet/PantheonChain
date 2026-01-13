// ParthenonChain - Node Infrastructure Implementation

#include "node.h"
#include "validation/validation.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace parthenon {
namespace node {

Node::Node(const std::string& data_dir, uint16_t port)
    : data_dir_(data_dir), port_(port), running_(false),
      is_syncing_(false), sync_target_height_(0) {
    
    // Initialize components
    chain_ = std::make_unique<chainstate::Chain>();
    mempool_ = std::make_unique<mempool::Mempool>();
    block_storage_ = std::make_unique<storage::BlockStorage>();
    utxo_storage_ = std::make_unique<storage::UTXOStorage>();
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
    
    // Initialize P2P network listener (stub implementation)
    std::cout << "Initializing P2P network on port " << port_ << std::endl;
    // In production: Create TCP listener socket, start accept loop
    
    // Start sync loop in background thread (stub implementation)
    std::cout << "Starting background sync thread" << std::endl;
    // In production: std::thread(&Node::SyncLoop, this).detach();
    
    running_ = true;
    is_syncing_ = true;
    
    std::cout << "Node started successfully" << std::endl;
    return true;
}

void Node::Stop() {
    if (!running_) {
        return;
    }
    
    std::cout << "Stopping node..." << std::endl;
    
    // Stop P2P network (stub implementation)
    std::cout << "Stopping P2P network..." << std::endl;
    // In production: Close all peer connections, stop listener
    
    // Save UTXO set to disk
    std::cout << "Saving UTXO set to disk..." << std::endl;
    auto& utxo_set = chain_->GetUTXOSet();
    utxo_storage_->SaveUTXOSet(utxo_set);
    
    // Close storage databases
    std::cout << "Closing storage databases..." << std::endl;
    block_storage_->Close();
    utxo_storage_->Close();
    
    // Stop sync thread (stub implementation)
    is_syncing_ = false;
    // In production: Join sync thread
    
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
                                   static_cast<double>(sync_target_height_)) * 100.0;
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
        return; // Already have this peer
    }
    
    PeerInfo info;
    info.address = address;
    info.port = port;
    info.version = 1;
    info.height = 0;
    info.is_connected = false;
    info.last_seen = 0;
    
    peers_[peer_id] = info;
    
    // TODO: Initiate connection to peer
    std::cout << "Added peer: " << peer_id << std::endl;
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
    
    error = validation::TransactionValidator::ValidateAgainstUTXO(tx, chain_->GetUTXOSet(), GetHeight());
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

std::optional<primitives::Block> Node::GetBlockByHash(
    const std::array<uint8_t, 32>& hash
) const {
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
    // Implement basic block synchronization loop
    std::cout << "Starting sync loop..." << std::endl;
    
    while (is_syncing_) {
        uint32_t current_height = GetHeight();
        
        // If we haven't reached the target, request more blocks
        if (current_height < sync_target_height_) {
            // In a real implementation, this would:
            // 1. Find peers with higher blocks
            // 2. Request blocks in batches
            // 3. Validate and apply blocks
            // 4. Update sync progress
            
            std::cout << "Syncing: " << current_height << "/" << sync_target_height_ << std::endl;
            
            // Simulate sync delay
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            // Caught up
            is_syncing_ = false;
            std::cout << "Sync complete at height " << current_height << std::endl;
        }
    }
}

void Node::RequestBlocks(const std::string& peer_id, uint32_t start_height, uint32_t count) {
    // Implement P2P block request
    std::cout << "Requesting " << count << " blocks starting at " << start_height
              << " from peer " << peer_id << std::endl;
    
    // In a real implementation:
    // 1. Create getblocks P2P message
    // 2. Send to specified peer
    // 3. Wait for block messages in response
    // 4. Validate each block
    // 5. Apply to chain if valid
}

bool Node::ValidateAndApplyBlock(const primitives::Block& block) {
    // Validate block structure
    if (!block.IsValid()) {
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
        error = validation::TransactionValidator::ValidateAgainstUTXO(tx, chain_->GetUTXOSet(), GetHeight());
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
    
    // Store block to disk
    uint32_t height = block.header.height;
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

void Node::BroadcastBlock(const primitives::Block& /* block */) {
    // TODO: Send block to all connected peers
    std::cout << "Broadcasting block at height " << GetHeight() << std::endl;
}

void Node::BroadcastTransaction(const primitives::Transaction& /* tx */) {
    // TODO: Send transaction to all connected peers
    std::cout << "Broadcasting transaction" << std::endl;
}

} // namespace node
} // namespace parthenon
