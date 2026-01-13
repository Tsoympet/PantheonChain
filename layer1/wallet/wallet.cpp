// ParthenonChain - Wallet Implementation

#include "wallet.h"
#include "crypto/sha256.h"
#include <algorithm>
#include <stdexcept>

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
    
    // Add inputs (without signatures initially)
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
        
        // Generate change address
        auto change_addr = GenerateAddress("change");
        
        primitives::TxOutput change_output;
        change_output.value = primitives::AssetAmount(asset, change);
        change_output.pubkey_script = change_addr.pubkey;
        
        tx.outputs.push_back(change_output);
    }
    
    // Sign inputs
    for (size_t i = 0; i < tx.inputs.size(); i++) {
        const auto& utxo = selected[i];
        
        // Find the private key for this UTXO
        // Extract pubkey from UTXO's pubkey_script (assuming it's the raw 32-byte pubkey)
        if (utxo.output.pubkey_script.size() != 32) {
            return std::nullopt; // Invalid pubkey format
        }
        
        // Find which address owns this UTXO
        uint64_t key_index = 0;
        bool found = false;
        for (const auto& addr : addresses_) {
            if (addr.pubkey == utxo.output.pubkey_script) {
                key_index = addr.index;
                found = true;
                break;
            }
        }
        
        if (!found) {
            return std::nullopt; // Don't have the key for this UTXO
        }
        
        // Get the private key
        auto it = keys_.find(key_index);
        if (it == keys_.end()) {
            // Derive the key if we don't have it cached
            keys_[key_index] = DeriveKey(key_index);
        }
        
        const auto& privkey = keys_[key_index];
        
        // Calculate signature hash for this input
        auto sighash = tx.GetSignatureHash(i);
        
        // Sign with Schnorr
        auto signature_opt = crypto::Schnorr::Sign(privkey, sighash.data());
        if (!signature_opt) {
            return std::nullopt; // Signature generation failed
        }
        
        // Set the signature script (just the 64-byte signature)
        tx.inputs[i].signature_script = std::vector<uint8_t>(
            signature_opt->begin(), signature_opt->end()
        );
    }
    
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
    // Clear existing UTXOs and resync from chain
    utxos_.clear();
    
    // For each wallet address:
    // 1. Query chainstate for UTXOs to that address
    // 2. Add to wallet UTXO tracking
    for (const auto& addr : addresses_) {
        // Get all UTXOs from the chainstate
        const auto& all_utxos = utxo_set.GetUTXOs();
        
        for (const auto& [outpoint, utxo_entry] : all_utxos) {
            // Check if this UTXO belongs to our wallet
            if (utxo_entry.output.pubkey_script == addr.pubkey) {
                // Add to wallet tracking
                WalletUTXO wallet_utxo;
                wallet_utxo.outpoint = outpoint;
                wallet_utxo.output = utxo_entry.output;
                wallet_utxo.height = utxo_entry.height;
                wallet_utxo.is_spent = false;
                utxos_.push_back(wallet_utxo);
            }
        }
    }
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

void Wallet::ProcessBlock(const primitives::Block& block, uint32_t height) {
    // Process all transactions in the block
    for (const auto& tx : block.transactions) {
        auto txid = tx.GetTxID();
        
        // Mark inputs as spent
        for (const auto& input : tx.inputs) {
            MarkSpent(input.prevout);
        }
        
        // Add new outputs that belong to us
        for (uint32_t vout = 0; vout < tx.outputs.size(); vout++) {
            const auto& output = tx.outputs[vout];
            
            if (IsOurPubkey(output.pubkey_script)) {
                primitives::OutPoint outpoint{txid, vout};
                AddUTXO(outpoint, output, height);
            }
        }
    }
}

bool Wallet::IsOurPubkey(const std::vector<uint8_t>& pubkey) const {
    // Check if this pubkey matches any of our addresses
    for (const auto& addr : addresses_) {
        if (addr.pubkey == pubkey) {
            return true;
        }
    }
    return false;
}

} // namespace wallet
} // namespace parthenon
