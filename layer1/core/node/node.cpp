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
}

Node::~Node() {
    Stop();
}

bool Node::Start() {
    if (running_) {
        return false;
    }
    
    std::cout << "Starting ParthenonChain node on port " << port_ << std::endl;
    
    // TODO: Load blockchain data from disk
    // TODO: Initialize P2P network listener
    // TODO: Start sync loop in background thread
    
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
    
    // TODO: Stop P2P network
    // TODO: Save blockchain state to disk
    // TODO: Stop sync thread
    
    running_ = false;
    is_syncing_ = false;
    
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
    validation::TransactionValidator validator(*chain_);
    if (!validator.ValidateTransaction(tx)) {
        std::cout << "Rejected invalid transaction" << std::endl;
        return false;
    }
    
    // Add to mempool
    if (!mempool_->AddTransaction(tx)) {
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
    // TODO: Implement block storage and retrieval
    // For now, return nullopt
    return std::nullopt;
}

std::optional<primitives::Block> Node::GetBlockByHash(
    const std::array<uint8_t, 32>& hash
) const {
    // TODO: Implement block hash index
    // For now, return nullopt
    return std::nullopt;
}

void Node::OnNewBlock(std::function<void(const primitives::Block&)> callback) {
    block_callbacks_.push_back(callback);
}

void Node::OnNewTransaction(std::function<void(const primitives::Transaction&)> callback) {
    tx_callbacks_.push_back(callback);
}

void Node::SyncLoop() {
    // TODO: Implement block synchronization loop
    // 1. Find best peer (highest height)
    // 2. Request missing blocks
    // 3. Validate and apply blocks
    // 4. Update sync status
}

void Node::RequestBlocks(const std::string& peer_id, uint32_t start_height, uint32_t count) {
    // TODO: Send P2P message to request blocks
    // For now, just log
    std::cout << "Requesting " << count << " blocks starting at " 
              << start_height << " from " << peer_id << std::endl;
}

bool Node::ValidateAndApplyBlock(const primitives::Block& block) {
    // Validate block structure
    if (!block.IsValid()) {
        return false;
    }
    
    // Validate all transactions
    validation::TransactionValidator validator(*chain_);
    for (const auto& tx : block.transactions) {
        // Skip validation for coinbase (first transaction)
        if (&tx == &block.transactions[0]) {
            continue;
        }
        
        if (!validator.ValidateTransaction(tx)) {
            return false;
        }
    }
    
    // Apply block to chain
    chainstate::BlockUndo undo;
    if (!chain_->ConnectBlock(block, undo)) {
        return false;
    }
    
    // Remove transactions from mempool
    for (const auto& tx : block.transactions) {
        mempool_->RemoveTransaction(tx.GetTxID());
    }
    
    return true;
}

void Node::BroadcastBlock(const primitives::Block& block) {
    // TODO: Send block to all connected peers
    std::cout << "Broadcasting block at height " << GetHeight() << std::endl;
}

void Node::BroadcastTransaction(const primitives::Transaction& tx) {
    // TODO: Send transaction to all connected peers
    std::cout << "Broadcasting transaction" << std::endl;
}

} // namespace node
} // namespace parthenon
