// ParthenonChain - EVM State Implementation

#include "state.h"
#include "crypto/sha256.h"
#include <algorithm>
#include <cstring>

namespace parthenon {
namespace evm {

std::optional<AccountState> WorldState::GetAccount(const Address& addr) const {
    auto it = accounts_.find(addr);
    if (it == accounts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void WorldState::SetAccount(const Address& addr, const AccountState& state) {
    accounts_[addr] = state;
}

bool WorldState::AccountExists(const Address& addr) const {
    return accounts_.find(addr) != accounts_.end();
}

uint256_t WorldState::GetStorage(const Address& addr, const uint256_t& key) const {
    auto storage_key = std::make_pair(addr, key);
    auto it = storage_.find(storage_key);
    if (it == storage_.end()) {
        return uint256_t{}; // Return zero if not set
    }
    return it->second;
}

void WorldState::SetStorage(const Address& addr, const uint256_t& key, const uint256_t& value) {
    auto storage_key = std::make_pair(addr, key);
    
    // Check if value is zero - if so, delete the entry
    bool is_zero = true;
    for (uint8_t byte : value) {
        if (byte != 0) {
            is_zero = false;
            break;
        }
    }
    
    if (is_zero) {
        storage_.erase(storage_key);
    } else {
        storage_[storage_key] = value;
    }
}

std::vector<uint8_t> WorldState::GetCode(const Address& addr) const {
    auto it = accounts_.find(addr);
    if (it == accounts_.end()) {
        return std::vector<uint8_t>();
    }
    return it->second.code;
}

void WorldState::SetCode(const Address& addr, const std::vector<uint8_t>& code) {
    auto& account = accounts_[addr];
    account.code = code;
    
    // Update code hash
    if (code.empty()) {
        account.code_hash = std::array<uint8_t, 32>{};
    } else {
        crypto::SHA256 hasher;
        hasher.Write(code.data(), code.size());
        auto hash = hasher.Finalize();
        std::memcpy(account.code_hash.data(), hash.data(), 32);
    }
}

uint256_t WorldState::GetBalance(const Address& addr) const {
    auto it = accounts_.find(addr);
    if (it == accounts_.end()) {
        return uint256_t{};
    }
    return it->second.balance;
}

void WorldState::SetBalance(const Address& addr, const uint256_t& balance) {
    accounts_[addr].balance = balance;
}

uint64_t WorldState::GetNonce(const Address& addr) const {
    auto it = accounts_.find(addr);
    if (it == accounts_.end()) {
        return 0;
    }
    return it->second.nonce;
}

void WorldState::SetNonce(const Address& addr, uint64_t nonce) {
    accounts_[addr].nonce = nonce;
}

void WorldState::DeleteAccount(const Address& addr) {
    // Remove account
    accounts_.erase(addr);
    
    // Remove all storage entries for this account
    auto it = storage_.begin();
    while (it != storage_.end()) {
        if (it->first.first == addr) {
            it = storage_.erase(it);
        } else {
            ++it;
        }
    }
}

std::array<uint8_t, 32> WorldState::CalculateStateRoot() const {
    // Simplified Merkle Patricia Trie state root calculation
    // For production, this would be a full MPT implementation
    
    crypto::SHA256 hasher;
    
    // Hash all accounts in sorted order for determinism
    std::vector<Address> addresses;
    for (const auto& [addr, _] : accounts_) {
        addresses.push_back(addr);
    }
    std::sort(addresses.begin(), addresses.end());
    
    for (const auto& addr : addresses) {
        const auto& account = accounts_.at(addr);
        
        // Hash address
        hasher.Write(addr.data(), addr.size());
        
        // Hash nonce
        uint8_t nonce_bytes[8];
        for (int i = 0; i < 8; i++) {
            nonce_bytes[i] = static_cast<uint8_t>((account.nonce >> (i * 8)) & 0xFF);
        }
        hasher.Write(nonce_bytes, 8);
        
        // Hash balance
        hasher.Write(account.balance.data(), 32);
        
        // Hash code hash
        hasher.Write(account.code_hash.data(), 32);
        
        // Hash storage root (simplified - hash all storage for this account)
        crypto::SHA256 storage_hasher;
        std::vector<uint256_t> keys;
        for (const auto& [key_pair, _] : storage_) {
            if (key_pair.first == addr) {
                keys.push_back(key_pair.second);
            }
        }
        std::sort(keys.begin(), keys.end());
        
        for (const auto& key : keys) {
            auto storage_key = std::make_pair(addr, key);
            const auto& value = storage_.at(storage_key);
            storage_hasher.Write(key.data(), 32);
            storage_hasher.Write(value.data(), 32);
        }
        
        std::array<uint8_t, 32> storage_root;
        auto storage_hash = storage_hasher.Finalize();
        std::memcpy(storage_root.data(), storage_hash.data(), 32);
        hasher.Write(storage_root.data(), 32);
    }
    
    std::array<uint8_t, 32> state_root;
    auto final_hash = hasher.Finalize();
    std::memcpy(state_root.data(), final_hash.data(), 32);
    return state_root;
}

WorldState::Snapshot WorldState::CreateSnapshot() const {
    Snapshot snapshot;
    snapshot.accounts = accounts_;
    snapshot.storage = storage_;
    return snapshot;
}

void WorldState::RestoreSnapshot(const Snapshot& snapshot) {
    accounts_ = snapshot.accounts;
    storage_ = snapshot.storage;
}

} // namespace evm
} // namespace parthenon
