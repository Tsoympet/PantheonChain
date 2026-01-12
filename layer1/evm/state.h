// ParthenonChain - EVM State Management
// Merkle Patricia Trie and state root implementation

#pragma once

#include <array>
#include <map>
#include <vector>
#include <cstdint>
#include <optional>

namespace parthenon {
namespace evm {

/**
 * 256-bit value (used for storage, balances, etc.)
 */
using uint256_t = std::array<uint8_t, 32>;

/**
 * EVM address (20 bytes)
 */
using Address = std::array<uint8_t, 20>;

/**
 * Convert uint64_t to uint256_t
 */
inline uint256_t ToUint256(uint64_t value) {
    uint256_t result{};
    for (int i = 0; i < 8; i++) {
        result[31 - i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }
    return result;
}

/**
 * Convert uint256_t to uint64_t (may truncate)
 */
inline uint64_t ToUint64(const uint256_t& value) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= static_cast<uint64_t>(value[31 - i]) << (i * 8);
    }
    return result;
}

/**
 * Account state in the world state
 */
struct AccountState {
    uint64_t nonce;
    uint256_t balance; // OBL balance for gas
    std::array<uint8_t, 32> code_hash;
    std::array<uint8_t, 32> storage_root;
    std::vector<uint8_t> code; // Contract bytecode
    
    AccountState() : nonce(0), balance{}, code_hash{}, storage_root{} {}
};

/**
 * Storage entry (key-value pair in contract storage)
 */
struct StorageEntry {
    uint256_t key;
    uint256_t value;
};

/**
 * World state - maintains all account states
 */
class WorldState {
public:
    WorldState() = default;
    
    /**
     * Get account state
     */
    std::optional<AccountState> GetAccount(const Address& addr) const;
    
    /**
     * Set account state
     */
    void SetAccount(const Address& addr, const AccountState& state);
    
    /**
     * Check if account exists
     */
    bool AccountExists(const Address& addr) const;
    
    /**
     * Get contract storage value
     */
    uint256_t GetStorage(const Address& addr, const uint256_t& key) const;
    
    /**
     * Set contract storage value
     */
    void SetStorage(const Address& addr, const uint256_t& key, const uint256_t& value);
    
    /**
     * Get contract code
     */
    std::vector<uint8_t> GetCode(const Address& addr) const;
    
    /**
     * Set contract code
     */
    void SetCode(const Address& addr, const std::vector<uint8_t>& code);
    
    /**
     * Get OBL balance
     */
    uint256_t GetBalance(const Address& addr) const;
    
    /**
     * Set OBL balance
     */
    void SetBalance(const Address& addr, const uint256_t& balance);
    
    /**
     * Get nonce
     */
    uint64_t GetNonce(const Address& addr) const;
    
    /**
     * Set nonce
     */
    void SetNonce(const Address& addr, uint64_t nonce);
    
    /**
     * Delete account
     */
    void DeleteAccount(const Address& addr);
    
    /**
     * Calculate state root (Merkle Patricia Trie root)
     */
    std::array<uint8_t, 32> CalculateStateRoot() const;
    
    /**
     * Create snapshot for reverting
     */
    struct Snapshot {
        std::map<Address, AccountState> accounts;
        std::map<std::pair<Address, uint256_t>, uint256_t> storage;
    };
    
    Snapshot CreateSnapshot() const;
    void RestoreSnapshot(const Snapshot& snapshot);
    
private:
    std::map<Address, AccountState> accounts_;
    std::map<std::pair<Address, uint256_t>, uint256_t> storage_;
};

/**
 * Log entry (emitted by LOG opcodes)
 */
struct LogEntry {
    Address address;
    std::vector<uint256_t> topics;
    std::vector<uint8_t> data;
};

} // namespace evm
} // namespace parthenon
