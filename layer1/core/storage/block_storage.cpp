// ParthenonChain - Block Storage Implementation

#include "block_storage.h"
#include <leveldb/write_batch.h>
#include <sstream>
#include <iomanip>

namespace parthenon {
namespace storage {

bool BlockStorage::Open(const std::string& db_path) {
    leveldb::Options options;
    options.create_if_missing = true;
    
    leveldb::DB* db_ptr;
    leveldb::Status status = leveldb::DB::Open(options, db_path, &db_ptr);
    
    if (!status.ok()) {
        return false;
    }
    
    db_.reset(db_ptr);
    return true;
}

void BlockStorage::Close() {
    db_.reset();
}

std::string BlockStorage::HeightKey(uint32_t height) {
    std::ostringstream oss;
    oss << "b" << std::setw(10) << std::setfill('0') << height;
    return oss.str();
}

std::string BlockStorage::HashKey(const std::array<uint8_t, 32>& hash) {
    std::string key = "h";
    for (uint8_t byte : hash) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", byte);
        key += buf;
    }
    return key;
}

std::string BlockStorage::SerializeBlock(const primitives::Block& block) {
    // Simple serialization (in production, use proper binary format)
    std::ostringstream oss;
    
    // Serialize header
    oss.write(reinterpret_cast<const char*>(&block.header.version), sizeof(block.header.version));
    oss.write(reinterpret_cast<const char*>(block.header.prev_block_hash.data()), 32);
    oss.write(reinterpret_cast<const char*>(block.header.merkle_root.data()), 32);
    oss.write(reinterpret_cast<const char*>(block.header.state_root.data()), 32);
    oss.write(reinterpret_cast<const char*>(&block.header.timestamp), sizeof(block.header.timestamp));
    oss.write(reinterpret_cast<const char*>(&block.header.height), sizeof(block.header.height));
    oss.write(reinterpret_cast<const char*>(block.header.target.data()), 32);
    oss.write(reinterpret_cast<const char*>(&block.header.nonce), sizeof(block.header.nonce));
    
    // Serialize transaction count
    uint32_t tx_count = static_cast<uint32_t>(block.transactions.size());
    oss.write(reinterpret_cast<const char*>(&tx_count), sizeof(tx_count));
    
    // Note: Full transaction serialization would go here
    // For now, we store the count to maintain structure
    
    return oss.str();
}

std::optional<primitives::Block> BlockStorage::DeserializeBlock(const std::string& data) {
    if (data.size() < sizeof(primitives::BlockHeader) + sizeof(uint32_t)) {
        return std::nullopt;
    }
    
    std::istringstream iss(data);
    primitives::Block block;
    
    // Deserialize header
    iss.read(reinterpret_cast<char*>(&block.header.version), sizeof(block.header.version));
    iss.read(reinterpret_cast<char*>(block.header.prev_block_hash.data()), 32);
    iss.read(reinterpret_cast<char*>(block.header.merkle_root.data()), 32);
    iss.read(reinterpret_cast<char*>(block.header.state_root.data()), 32);
    iss.read(reinterpret_cast<char*>(&block.header.timestamp), sizeof(block.header.timestamp));
    iss.read(reinterpret_cast<char*>(&block.header.height), sizeof(block.header.height));
    iss.read(reinterpret_cast<char*>(block.header.target.data()), 32);
    iss.read(reinterpret_cast<char*>(&block.header.nonce), sizeof(block.header.nonce));
    
    // Deserialize transaction count
    uint32_t tx_count;
    iss.read(reinterpret_cast<char*>(&tx_count), sizeof(tx_count));
    
    // Note: Full transaction deserialization would go here
    
    return block;
}

bool BlockStorage::StoreBlock(const primitives::Block& block, uint32_t height) {
    if (!db_) {
        return false;
    }
    
    leveldb::WriteBatch batch;
    
    // Store block by height
    std::string height_key = HeightKey(height);
    std::string block_data = SerializeBlock(block);
    batch.Put(height_key, block_data);
    
    // Store hash -> height mapping
    auto block_hash = block.GetHash();
    std::string hash_key = HashKey(block_hash);
    std::string height_str = std::to_string(height);
    batch.Put(hash_key, height_str);
    
    // Write batch atomically
    leveldb::WriteOptions options;
    leveldb::Status status = db_->Write(options, &batch);
    
    return status.ok();
}

std::optional<primitives::Block> BlockStorage::GetBlockByHeight(uint32_t height) {
    if (!db_) {
        return std::nullopt;
    }
    
    std::string key = HeightKey(height);
    std::string value;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    if (!status.ok()) {
        return std::nullopt;
    }
    
    return DeserializeBlock(value);
}

std::optional<primitives::Block> BlockStorage::GetBlockByHash(const std::array<uint8_t, 32>& hash) {
    if (!db_) {
        return std::nullopt;
    }
    
    // First get height from hash
    std::string hash_key = HashKey(hash);
    std::string height_str;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), hash_key, &height_str);
    if (!status.ok()) {
        return std::nullopt;
    }
    
    // Then get block by height
    uint32_t height = std::stoul(height_str);
    return GetBlockByHeight(height);
}

uint32_t BlockStorage::GetHeight() {
    if (!db_) {
        return 0;
    }
    
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), "meta:height", &value);
    
    if (!status.ok()) {
        return 0;
    }
    
    return std::stoul(value);
}

bool BlockStorage::UpdateChainTip(uint32_t height, const std::array<uint8_t, 32>& best_hash) {
    if (!db_) {
        return false;
    }
    
    leveldb::WriteBatch batch;
    
    // Update height
    batch.Put("meta:height", std::to_string(height));
    
    // Update best hash
    std::string hash_hex;
    for (uint8_t byte : best_hash) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", byte);
        hash_hex += buf;
    }
    batch.Put("meta:best_hash", hash_hex);
    
    leveldb::WriteOptions options;
    leveldb::Status status = db_->Write(options, &batch);
    
    return status.ok();
}

} // namespace storage
} // namespace parthenon
