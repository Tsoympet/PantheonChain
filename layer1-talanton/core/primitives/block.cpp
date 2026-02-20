// ParthenonChain - Block Implementation
// Consensus-critical: Block serialization, validation, and Merkle tree

#include "block.h"

#include "consensus/difficulty.h"

#include <cstddef>
#include <cstring>

namespace parthenon {
namespace primitives {

static bool ReadCompactSizeChecked(const uint8_t*& input, const uint8_t* end, uint64_t& size) {
    if (input >= end) {
        return false;
    }

    uint8_t first = *input++;
    if (first < 253) {
        size = first;
        return true;
    }

    if (first == 253) {
        if (end - input < 2) {
            return false;
        }
        size = input[0] | (static_cast<uint64_t>(input[1]) << 8);
        if (size < 253) {
            return false;
        }
        input += 2;
        return true;
    }

    if (first == 254) {
        if (end - input < 4) {
            return false;
        }
        size = input[0] | (static_cast<uint64_t>(input[1]) << 8) |
               (static_cast<uint64_t>(input[2]) << 16) | (static_cast<uint64_t>(input[3]) << 24);
        if (size <= 0xFFFF) {
            return false;
        }
        input += 4;
        return true;
    }

    if (end - input < 8) {
        return false;
    }

    size = 0;
    for (int i = 0; i < 8; i++) {
        size |= static_cast<uint64_t>(input[i]) << (i * 8);
    }
    if (size <= 0xFFFFFFFFULL) {
        return false;
    }
    input += 8;
    return true;
}

// BlockHeader methods
std::vector<uint8_t> BlockHeader::Serialize() const {
    std::vector<uint8_t> result;
    result.reserve(104);  // Extended header: 80 + 24 = 104 bytes

    // Version (4 bytes)
    result.push_back(static_cast<uint8_t>(version));
    result.push_back(static_cast<uint8_t>(version >> 8));
    result.push_back(static_cast<uint8_t>(version >> 16));
    result.push_back(static_cast<uint8_t>(version >> 24));

    // Previous block hash (32 bytes)
    result.insert(result.end(), prev_block_hash.begin(), prev_block_hash.end());

    // Merkle root (32 bytes)
    result.insert(result.end(), merkle_root.begin(), merkle_root.end());

    // Timestamp (4 bytes)
    result.push_back(static_cast<uint8_t>(timestamp));
    result.push_back(static_cast<uint8_t>(timestamp >> 8));
    result.push_back(static_cast<uint8_t>(timestamp >> 16));
    result.push_back(static_cast<uint8_t>(timestamp >> 24));

    // Bits (4 bytes)
    result.push_back(static_cast<uint8_t>(bits));
    result.push_back(static_cast<uint8_t>(bits >> 8));
    result.push_back(static_cast<uint8_t>(bits >> 16));
    result.push_back(static_cast<uint8_t>(bits >> 24));

    // Nonce (4 bytes)
    result.push_back(static_cast<uint8_t>(nonce));
    result.push_back(static_cast<uint8_t>(nonce >> 8));
    result.push_back(static_cast<uint8_t>(nonce >> 16));
    result.push_back(static_cast<uint8_t>(nonce >> 24));

    // Base fee per gas (8 bytes)
    result.push_back(static_cast<uint8_t>(base_fee_per_gas));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 8));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 16));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 24));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 32));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 40));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 48));
    result.push_back(static_cast<uint8_t>(base_fee_per_gas >> 56));

    // Gas used (8 bytes)
    result.push_back(static_cast<uint8_t>(gas_used));
    result.push_back(static_cast<uint8_t>(gas_used >> 8));
    result.push_back(static_cast<uint8_t>(gas_used >> 16));
    result.push_back(static_cast<uint8_t>(gas_used >> 24));
    result.push_back(static_cast<uint8_t>(gas_used >> 32));
    result.push_back(static_cast<uint8_t>(gas_used >> 40));
    result.push_back(static_cast<uint8_t>(gas_used >> 48));
    result.push_back(static_cast<uint8_t>(gas_used >> 56));

    // Gas limit (8 bytes)
    result.push_back(static_cast<uint8_t>(gas_limit));
    result.push_back(static_cast<uint8_t>(gas_limit >> 8));
    result.push_back(static_cast<uint8_t>(gas_limit >> 16));
    result.push_back(static_cast<uint8_t>(gas_limit >> 24));
    result.push_back(static_cast<uint8_t>(gas_limit >> 32));
    result.push_back(static_cast<uint8_t>(gas_limit >> 40));
    result.push_back(static_cast<uint8_t>(gas_limit >> 48));
    result.push_back(static_cast<uint8_t>(gas_limit >> 56));

    return result;
}

BlockHeader BlockHeader::Deserialize(const uint8_t* data) {
    BlockHeader header;

    // Version
    header.version = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                     (static_cast<uint32_t>(data[2]) << 16) |
                     (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    // Previous block hash
    std::copy(data, data + 32, header.prev_block_hash.begin());
    data += 32;

    // Merkle root
    std::copy(data, data + 32, header.merkle_root.begin());
    data += 32;

    // Timestamp
    header.timestamp = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                       (static_cast<uint32_t>(data[2]) << 16) |
                       (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    // Bits
    header.bits = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                  (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    // Nonce
    header.nonce = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                   (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    // Base fee per gas (8 bytes)
    header.base_fee_per_gas =
        data[0] | (static_cast<uint64_t>(data[1]) << 8) | (static_cast<uint64_t>(data[2]) << 16) |
        (static_cast<uint64_t>(data[3]) << 24) | (static_cast<uint64_t>(data[4]) << 32) |
        (static_cast<uint64_t>(data[5]) << 40) | (static_cast<uint64_t>(data[6]) << 48) |
        (static_cast<uint64_t>(data[7]) << 56);
    data += 8;

    // Gas used (8 bytes)
    header.gas_used =
        data[0] | (static_cast<uint64_t>(data[1]) << 8) | (static_cast<uint64_t>(data[2]) << 16) |
        (static_cast<uint64_t>(data[3]) << 24) | (static_cast<uint64_t>(data[4]) << 32) |
        (static_cast<uint64_t>(data[5]) << 40) | (static_cast<uint64_t>(data[6]) << 48) |
        (static_cast<uint64_t>(data[7]) << 56);
    data += 8;

    // Gas limit (8 bytes)
    header.gas_limit =
        data[0] | (static_cast<uint64_t>(data[1]) << 8) | (static_cast<uint64_t>(data[2]) << 16) |
        (static_cast<uint64_t>(data[3]) << 24) | (static_cast<uint64_t>(data[4]) << 32) |
        (static_cast<uint64_t>(data[5]) << 40) | (static_cast<uint64_t>(data[6]) << 48) |
        (static_cast<uint64_t>(data[7]) << 56);

    return header;
}

std::array<uint8_t, 32> BlockHeader::GetHash() const {
    auto serialized = Serialize();
    return crypto::SHA256d::Hash256d(serialized);
}

bool BlockHeader::MeetsDifficultyTarget() const {
    auto hash = GetHash();
    return consensus::Difficulty::CheckProofOfWork(hash, bits);
}

// Block methods
std::vector<uint8_t> Block::Serialize() const {
    std::vector<uint8_t> result;

    // Header (104 bytes with EVM fields)
    auto header_bytes = header.Serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());

    // Transaction count
    WriteCompactSize(result, transactions.size());

    // Transactions
    for (const auto& tx : transactions) {
        auto tx_bytes = tx.Serialize();
        result.insert(result.end(), tx_bytes.begin(), tx_bytes.end());
    }

    return result;
}

std::optional<Block> Block::Deserialize(const uint8_t* data, size_t len) {
    // Minimum block size: 104 (extended header) + 1 (compact size) = 105 bytes
    if (len < 105)
        return std::nullopt;

    Block block;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    // Header (104 bytes with EVM fields)
    block.header = BlockHeader::Deserialize(ptr);
    ptr += 104;  // Extended header size

    // Transaction count
    uint64_t tx_count = 0;
    if (!ReadCompactSizeChecked(ptr, end, tx_count)) {
        return std::nullopt;
    }
    if (tx_count == 0 || tx_count > 1000000)
        return std::nullopt;  // Sanity check

    // Transactions
    for (uint64_t i = 0; i < tx_count; i++) {
        size_t remaining = static_cast<size_t>(end - ptr);
        auto tx = Transaction::Deserialize(ptr, remaining);
        if (!tx)
            return std::nullopt;
        block.transactions.push_back(*tx);

        // Advance pointer (simplified - need to track actual bytes read)
        auto tx_bytes = tx->Serialize();
        if (tx_bytes.size() > remaining)
            return std::nullopt;
        ptr += tx_bytes.size();
    }

    if (ptr != end) {
        return std::nullopt;
    }

    return block;
}

std::array<uint8_t, 32> Block::CalculateMerkleRoot() const {
    return MerkleTree::CalculateRoot(transactions);
}

bool Block::IsValid() const {
    // Must have at least one transaction
    if (transactions.empty()) {
        return false;
    }

    // First transaction must be coinbase
    if (!transactions[0].IsCoinbase()) {
        return false;
    }

    // Only first transaction can be coinbase
    for (size_t i = 1; i < transactions.size(); i++) {
        if (transactions[i].IsCoinbase()) {
            return false;
        }
    }

    // All transactions must be valid
    for (const auto& tx : transactions) {
        if (!tx.IsValid()) {
            return false;
        }
    }

    // Merkle root must match
    auto calculated_root = CalculateMerkleRoot();
    if (calculated_root != header.merkle_root) {
        return false;
    }

    // Block hash must meet difficulty target
    if (!header.MeetsDifficultyTarget()) {
        return false;
    }

    return true;
}

// Merkle tree implementation
std::array<uint8_t, 32> MerkleTree::HashNodes(const std::array<uint8_t, 32>& left,
                                              const std::array<uint8_t, 32>& right) {
    std::vector<uint8_t> combined;
    combined.reserve(64);
    combined.insert(combined.end(), left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    return crypto::SHA256d::Hash256d(combined);
}

std::array<uint8_t, 32>
MerkleTree::CalculateRoot(const std::vector<std::array<uint8_t, 32>>& hashes) {
    if (hashes.empty()) {
        return std::array<uint8_t, 32>{};  // Empty merkle root
    }

    if (hashes.size() == 1) {
        return hashes[0];
    }

    // Build tree bottom-up
    std::vector<std::array<uint8_t, 32>> current_level = hashes;

    while (current_level.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next_level;

        for (size_t i = 0; i < current_level.size(); i += 2) {
            if (i + 1 < current_level.size()) {
                // Hash pair
                next_level.push_back(HashNodes(current_level[i], current_level[i + 1]));
            } else {
                // Odd number of nodes - duplicate last one
                next_level.push_back(HashNodes(current_level[i], current_level[i]));
            }
        }

        current_level = next_level;
    }

    return current_level[0];
}

std::array<uint8_t, 32> MerkleTree::CalculateRoot(const std::vector<Transaction>& transactions) {
    std::vector<std::array<uint8_t, 32>> tx_hashes;
    tx_hashes.reserve(transactions.size());

    for (const auto& tx : transactions) {
        tx_hashes.push_back(tx.GetTxID());
    }

    return CalculateRoot(tx_hashes);
}

}  // namespace primitives
}  // namespace parthenon
