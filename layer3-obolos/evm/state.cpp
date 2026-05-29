// ParthenonChain - EVM State Implementation

#include "state.h"

#include "crypto/sha256.h"

#include "mpt.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace evm {

void WorldState::MarkAccountDirty(const Address& addr) {
    dirty_storage_roots_.insert(addr);
    state_root_dirty_ = true;
}

std::optional<AccountState> WorldState::GetAccount(const Address& addr) const {
    auto it = accounts_.find(addr);
    if (it == accounts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void WorldState::SetAccount(const Address& addr, const AccountState& state) {
    accounts_[addr] = state;
    MarkAccountDirty(addr);
}

bool WorldState::AccountExists(const Address& addr) const {
    return accounts_.find(addr) != accounts_.end();
}

uint256_t WorldState::GetStorage(const Address& addr, const uint256_t& key) const {
    auto storage_key = std::make_pair(addr, key);
    auto it = storage_.find(storage_key);
    if (it == storage_.end()) {
        return uint256_t{};  // Return zero if not set
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
    MarkAccountDirty(addr);
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
    MarkAccountDirty(addr);
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
    state_root_dirty_ = true;
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
    state_root_dirty_ = true;
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
    storage_roots_.erase(addr);
    dirty_storage_roots_.erase(addr);
    state_root_dirty_ = true;
}

std::array<uint8_t, 32> WorldState::CalculateStorageRoot(const Address& addr) const {
    auto cached = storage_roots_.find(addr);
    if (cached != storage_roots_.end() && dirty_storage_roots_.count(addr) == 0) {
        return cached->second;
    }

    MerklePatriciaTrie storage_trie;
    auto it = storage_.lower_bound(std::make_pair(addr, uint256_t{}));
    while (it != storage_.end() && it->first.first == addr) {
        std::vector<uint8_t> storage_key(it->first.second.begin(), it->first.second.end());
        std::vector<uint8_t> storage_value(it->second.begin(), it->second.end());
        storage_trie.Put(storage_key, storage_value);
        ++it;
    }

    auto root = storage_trie.GetRootHash();
    storage_roots_[addr] = root;
    dirty_storage_roots_.erase(addr);
    return root;
}

std::array<uint8_t, 32> WorldState::CalculateStateRoot() const {
    if (!state_root_dirty_ && cached_state_root_) {
        return *cached_state_root_;
    }

    // Production-grade Merkle Patricia Trie state root calculation
    // Implements Ethereum-compatible MPT structure
    MerklePatriciaTrie trie;

    // Insert all accounts into the MPT
    for (const auto& [addr, account] : accounts_) {
        // Create account key (address as bytes)
        std::vector<uint8_t> account_key(addr.begin(), addr.end());

        // Build account value (nonce + balance + code_hash + storage_root)
        std::vector<uint8_t> account_value;

        // Nonce (8 bytes, little-endian)
        for (int i = 0; i < 8; i++) {
            account_value.push_back(static_cast<uint8_t>((account.nonce >> (i * 8)) & 0xFF));
        }

        // Balance (32 bytes)
        account_value.insert(account_value.end(), account.balance.begin(), account.balance.end());

        // Code hash (32 bytes)
        account_value.insert(account_value.end(), account.code_hash.begin(),
                             account.code_hash.end());

        auto storage_root = CalculateStorageRoot(addr);
        account_value.insert(account_value.end(), storage_root.begin(), storage_root.end());

        // Insert account into main trie
        trie.Put(account_key, account_value);
    }

    // Return the root hash of the account trie
    cached_state_root_ = trie.GetRootHash();
    state_root_dirty_ = false;
    return *cached_state_root_;
}

WorldState::Snapshot WorldState::CreateSnapshot() const {
    Snapshot snapshot;
    snapshot.accounts = accounts_;
    snapshot.storage = storage_;
    snapshot.storage_roots = storage_roots_;
    snapshot.dirty_storage_roots = dirty_storage_roots_;
    snapshot.state_root = cached_state_root_;
    snapshot.state_root_dirty = state_root_dirty_;
    return snapshot;
}

void WorldState::RestoreSnapshot(const Snapshot& snapshot) {
    accounts_ = snapshot.accounts;
    storage_ = snapshot.storage;
    storage_roots_ = snapshot.storage_roots;
    dirty_storage_roots_ = snapshot.dirty_storage_roots;
    cached_state_root_ = snapshot.state_root;
    state_root_dirty_ = snapshot.state_root_dirty;
}

}  // namespace evm
}  // namespace parthenon
