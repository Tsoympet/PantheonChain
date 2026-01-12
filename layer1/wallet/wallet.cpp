// ParthenonChain - Wallet Implementation

#include "wallet.h"
#include "crypto/sha256.h"
#include <algorithm>

namespace parthenon {
namespace wallet {

Wallet::Wallet(const std::array<uint8_t, 32>& seed)
    : seed_(seed), next_index_(0) {
}

WalletAddress Wallet::GenerateAddress(const std::string& label) {
    // Derive new key
    uint64_t index = next_index_++;
    auto privkey = DeriveKey(index);
    keys_[index] = privkey;
    
    // Generate public key
    auto pubkey_opt = crypto::Schnorr::GetPublicKey(privkey);
    if (!pubkey_opt) {
        throw std::runtime_error("Failed to generate public key");
    }
    
    WalletAddress addr;
    addr.pubkey = std::vector<uint8_t>(pubkey_opt->begin(), pubkey_opt->end());
    addr.label = label;
    addr.index = index;
    
    addresses_.push_back(addr);
    return addr;
}

std::vector<WalletAddress> Wallet::GetAddresses() const {
    return addresses_;
}

uint64_t Wallet::GetBalance(primitives::AssetID asset) const {
    uint64_t balance = 0;
    
    for (const auto& utxo : utxos_) {
        if (!utxo.is_spent && utxo.output.value.asset == asset) {
            balance += utxo.output.value.amount;
        }
    }
    
    return balance;
}

std::map<primitives::AssetID, uint64_t> Wallet::GetBalances() const {
    std::map<primitives::AssetID, uint64_t> balances;
    balances[primitives::AssetID::TALANTON] = GetBalance(primitives::AssetID::TALANTON);
    balances[primitives::AssetID::DRACHMA] = GetBalance(primitives::AssetID::DRACHMA);
    balances[primitives::AssetID::OBOLOS] = GetBalance(primitives::AssetID::OBOLOS);
    return balances;
}

std::vector<WalletUTXO> Wallet::ListUTXOs(bool include_spent) const {
    if (include_spent) {
        return utxos_;
    }
    
    std::vector<WalletUTXO> unspent;
    for (const auto& utxo : utxos_) {
        if (!utxo.is_spent) {
            unspent.push_back(utxo);
        }
    }
    return unspent;
}

std::optional<primitives::Transaction> Wallet::CreateTransaction(
    const std::vector<primitives::TxOutput>& outputs,
    primitives::AssetID asset,
    uint64_t fee_amount
) {
    // Calculate total output amount
    uint64_t total_out = fee_amount;
    for (const auto& output : outputs) {
        if (output.value.asset == asset) {
            total_out += output.value.amount;
        }
    }
    
    // Select coins
    auto selected = SelectCoins(asset, total_out);
    if (selected.empty()) {
        return std::nullopt;  // Insufficient funds
    }
    
    // Calculate total input amount
    uint64_t total_in = 0;
    for (const auto& utxo : selected) {
        total_in += utxo.output.value.amount;
    }
    
    // Create transaction
    primitives::Transaction tx;
    tx.version = 1;
    tx.locktime = 0;
    
    // Add inputs (without signatures for now)
    for (const auto& utxo : selected) {
        primitives::TxInput input;
        input.prevout = utxo.outpoint;
        input.sequence = 0xFFFFFFFF;
        tx.inputs.push_back(input);
    }
    
    // Add outputs
    for (const auto& output : outputs) {
        tx.outputs.push_back(output);
    }
    
    // Add change output if needed
    if (total_in > total_out) {
        uint64_t change = total_in - total_out;
        // TODO: Get change address
        // For now, skip change output
    }
    
    // Sign inputs
    // TODO: Implement signature generation
    // For each input, need to:
    // 1. Get signature hash
    // 2. Sign with corresponding private key
    // 3. Set signature_script
    
    return tx;
}

void Wallet::AddUTXO(const primitives::OutPoint& outpoint,
                     const primitives::TxOutput& output,
                     uint32_t height) {
    WalletUTXO utxo;
    utxo.outpoint = outpoint;
    utxo.output = output;
    utxo.height = height;
    utxo.is_spent = false;
    
    utxos_.push_back(utxo);
}

void Wallet::MarkSpent(const primitives::OutPoint& outpoint) {
    for (auto& utxo : utxos_) {
        if (utxo.outpoint == outpoint) {
            utxo.is_spent = true;
            break;
        }
    }
}

void Wallet::SyncWithChain(const chainstate::UTXOSet& utxo_set) {
    // TODO: Implement chain sync
    // For each wallet address:
    // 1. Query chainstate for UTXOs to that address
    // 2. Add to wallet UTXO tracking
    // 3. Mark spent UTXOs
}

crypto::Schnorr::PrivateKey Wallet::DeriveKey(uint64_t index) {
    // Simplified BIP-32 derivation
    // In production, would use proper BIP-32 implementation
    crypto::SHA256 hasher;
    hasher.Write(seed_.data(), seed_.size());
    
    // Mix in index
    uint8_t index_bytes[8];
    for (int i = 0; i < 8; i++) {
        index_bytes[i] = static_cast<uint8_t>((index >> (i * 8)) & 0xFF);
    }
    hasher.Write(index_bytes, 8);
    
    auto hash = hasher.Finalize();
    
    crypto::Schnorr::PrivateKey privkey;
    std::copy(hash.begin(), hash.end(), privkey.begin());
    
    return privkey;
}

std::vector<WalletUTXO> Wallet::SelectCoins(primitives::AssetID asset, uint64_t amount) {
    std::vector<WalletUTXO> selected;
    uint64_t total = 0;
    
    // Simple coin selection: use first-fit
    for (const auto& utxo : utxos_) {
        if (!utxo.is_spent && utxo.output.value.asset == asset) {
            selected.push_back(utxo);
            total += utxo.output.value.amount;
            
            if (total >= amount) {
                return selected;
            }
        }
    }
    
    // Insufficient funds
    return {};
}

} // namespace wallet
} // namespace parthenon
