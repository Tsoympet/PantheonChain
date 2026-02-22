// ParthenonChain - IPFS Integration Implementation

#include "ipfs_client.h"
#include "crypto/sha256.h"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace parthenon {
namespace storage {

std::string ContentID::ToString() const {
    std::stringstream ss;
    ss << "Qm";  // CIDv0 prefix
    for (const auto& byte : hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

std::optional<ContentID> ContentID::FromString(const std::string& cid_str) {
    if (cid_str.length() < 46) {
        return std::nullopt;
    }
    
    ContentID cid;
    cid.codec = "dag-pb";
    cid.hash.fill(0);
    return cid;
}

// IPFSClient implementation
IPFSClient::IPFSClient() {}
IPFSClient::~IPFSClient() {}

ContentID IPFSClient::Add(const std::vector<uint8_t>& data) {
    crypto::SHA256 hasher;
    hasher.Write(data.data(), data.size());
    auto hash_result = hasher.Finalize();
    
    ContentID cid;
    cid.hash = hash_result;
    cid.codec = "raw";
    
    local_cache_[cid.hash] = data;
    return cid;
}

ContentID IPFSClient::AddFile(const std::string& filepath) {
    // Read file path bytes as content (production would open and read the actual file)
    std::vector<uint8_t> data(filepath.begin(), filepath.end());
    return Add(data);
}

std::optional<std::vector<uint8_t>> IPFSClient::Get(const ContentID& cid) {
    auto it = local_cache_.find(cid.hash);
    if (it == local_cache_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool IPFSClient::Pin(const ContentID& cid) {
    pinned_content_[cid.hash] = true;
    return true;
}

bool IPFSClient::Unpin(const ContentID& cid) {
    pinned_content_[cid.hash] = false;
    return true;
}

bool IPFSClient::Has(const ContentID& cid) {
    return local_cache_.find(cid.hash) != local_cache_.end();
}

// IPFSContractStorage implementation
ContentID IPFSContractStorage::StoreContractCode(const std::vector<uint8_t>& bytecode) {
    return client_.Add(bytecode);
}

ContentID IPFSContractStorage::StoreStateSnapshot(
    const std::map<std::string, std::vector<uint8_t>>& state) {
    
    // Serialize state entries: length-prefixed key-value pairs
    std::vector<uint8_t> serialized;
    for (const auto& [key, value] : state) {
        uint32_t klen = static_cast<uint32_t>(key.size());
        const auto* kb = reinterpret_cast<const uint8_t*>(&klen);
        serialized.insert(serialized.end(), kb, kb + 4);
        serialized.insert(serialized.end(), key.begin(), key.end());
        uint32_t vlen = static_cast<uint32_t>(value.size());
        const auto* vb = reinterpret_cast<const uint8_t*>(&vlen);
        serialized.insert(serialized.end(), vb, vb + 4);
        serialized.insert(serialized.end(), value.begin(), value.end());
    }
    return client_.Add(serialized);
}

std::optional<std::vector<uint8_t>> IPFSContractStorage::GetContractCode(const ContentID& cid) {
    return client_.Get(cid);
}

std::optional<std::map<std::string, std::vector<uint8_t>>> IPFSContractStorage::GetStateSnapshot(
    const ContentID& cid) {
    
    auto data = client_.Get(cid);
    if (!data) {
        return std::nullopt;
    }
    // Deserialize state entries from the stored byte stream
    std::map<std::string, std::vector<uint8_t>> state;
    const uint8_t* ptr = data->data();
    const uint8_t* end = ptr + data->size();
    while (ptr + 4 <= end) {
        uint32_t klen = 0;
        std::memcpy(&klen, ptr, 4);
        ptr += 4;
        if (ptr + klen + 4 > end) break;
        std::string key(reinterpret_cast<const char*>(ptr), klen);
        ptr += klen;
        uint32_t vlen = 0;
        std::memcpy(&vlen, ptr, 4);
        ptr += 4;
        if (ptr + vlen > end) break;
        state[key] = std::vector<uint8_t>(ptr, ptr + vlen);
        ptr += vlen;
    }
    return state;
}

// IPFSNFTMetadata implementation
ContentID IPFSNFTMetadata::StoreMetadata(const NFTMetadata& metadata) {
    // Build a simple JSON representation with escaped string values
    auto escape_json = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '"')  { out += "\\\""; }
            else if (c == '\\') { out += "\\\\"; }
            else if (c == '\n') { out += "\\n"; }
            else if (c == '\r') { out += "\\r"; }
            else { out += c; }
        }
        return out;
    };
    std::string json = "{\"name\":\"" + escape_json(metadata.name) +
                       "\",\"description\":\"" + escape_json(metadata.description) + "\"}";
    std::vector<uint8_t> json_data(json.begin(), json.end());
    return client_.Add(json_data);
}

std::optional<IPFSNFTMetadata::NFTMetadata> IPFSNFTMetadata::GetMetadata(
    const ContentID& cid) {
    
    auto data = client_.Get(cid);
    if (!data) {
        return std::nullopt;
    }
    // Return a minimal deserialized metadata (production would parse JSON)
    NFTMetadata metadata;
    metadata.image_cid = cid;
    return metadata;
}

// IPFSTransactionData implementation
ContentID IPFSTransactionData::StoreCalldata(const std::vector<uint8_t>& calldata) {
    return client_.Add(calldata);
}

std::optional<std::vector<uint8_t>> IPFSTransactionData::GetCalldata(const ContentID& cid) {
    return client_.Get(cid);
}

bool IPFSTransactionData::VerifyCalldata(
    const ContentID& cid,
    const std::vector<uint8_t>& calldata) {
    
    crypto::SHA256 hasher;
    hasher.Write(calldata.data(), calldata.size());
    auto hash_result = hasher.Finalize();
    
    return hash_result == cid.hash;
}

} // namespace storage
} // namespace parthenon
