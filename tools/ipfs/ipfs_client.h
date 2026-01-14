// ParthenonChain - IPFS Integration
// Interplanetary File System for decentralized storage

#pragma once

#include <vector>
#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <array>

namespace parthenon {
namespace storage {

/**
 * IPFS Content Identifier (CID)
 */
struct ContentID {
    std::array<uint8_t, 32> hash;
    std::string codec;  // dag-pb, raw, etc.
    
    std::string ToString() const;
    static std::optional<ContentID> FromString(const std::string& cid_str);
};

/**
 * IPFS Client
 * Interface to IPFS network
 */
class IPFSClient {
public:
    IPFSClient();
    ~IPFSClient();
    
    /**
     * Add data to IPFS
     */
    ContentID Add(const std::vector<uint8_t>& data);
    
    /**
     * Add file to IPFS
     */
    ContentID AddFile(const std::string& filepath);
    
    /**
     * Get data from IPFS
     */
    std::optional<std::vector<uint8_t>> Get(const ContentID& cid);
    
    /**
     * Pin content to prevent garbage collection
     */
    bool Pin(const ContentID& cid);
    
    /**
     * Unpin content
     */
    bool Unpin(const ContentID& cid);
    
    /**
     * Check if content is available
     */
    bool Has(const ContentID& cid);
    
private:
    std::map<std::array<uint8_t, 32>, std::vector<uint8_t>> local_cache_;
    std::map<std::array<uint8_t, 32>, bool> pinned_content_;
};

/**
 * IPFS Smart Contract Storage
 * Store large contract data on IPFS
 */
class IPFSContractStorage {
public:
    /**
     * Store contract bytecode on IPFS
     */
    ContentID StoreContractCode(const std::vector<uint8_t>& bytecode);
    
    /**
     * Store contract state snapshot
     */
    ContentID StoreStateSnapshot(const std::map<std::string, std::vector<uint8_t>>& state);
    
    /**
     * Retrieve contract code
     */
    std::optional<std::vector<uint8_t>> GetContractCode(const ContentID& cid);
    
    /**
     * Retrieve state snapshot
     */
    std::optional<std::map<std::string, std::vector<uint8_t>>> GetStateSnapshot(const ContentID& cid);
    
private:
    IPFSClient client_;
};

/**
 * IPFS NFT Metadata
 * Store NFT metadata on IPFS
 */
class IPFSNFTMetadata {
public:
    struct NFTMetadata {
        std::string name;
        std::string description;
        ContentID image_cid;
        std::map<std::string, std::string> attributes;
    };
    
    /**
     * Store NFT metadata
     */
    ContentID StoreMetadata(const NFTMetadata& metadata);
    
    /**
     * Retrieve NFT metadata
     */
    std::optional<NFTMetadata> GetMetadata(const ContentID& cid);
    
private:
    IPFSClient client_;
};

/**
 * IPFS Transaction Data
 * Store large transaction data off-chain
 */
class IPFSTransactionData {
public:
    /**
     * Store transaction calldata on IPFS
     */
    ContentID StoreCalldata(const std::vector<uint8_t>& calldata);
    
    /**
     * Get calldata from IPFS
     */
    std::optional<std::vector<uint8_t>> GetCalldata(const ContentID& cid);
    
    /**
     * Verify calldata matches CID
     */
    bool VerifyCalldata(
        const ContentID& cid,
        const std::vector<uint8_t>& calldata
    );
    
private:
    IPFSClient client_;
};

} // namespace storage
} // namespace parthenon
