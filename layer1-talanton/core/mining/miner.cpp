// ParthenonChain - Mining Module Implementation
// Proof-of-Work block mining and template construction

#include "miner.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <chrono>
#include <cstring>

namespace parthenon {
namespace mining {

Miner::Miner(chainstate::ChainState& chainstate, const std::vector<uint8_t>& coinbase_pubkey)
    : chainstate_(chainstate),
      coinbase_pubkey_(coinbase_pubkey),
      is_mining_(false),
      hashrate_(0),
      total_hashes_(0) {}

std::optional<BlockTemplate> Miner::CreateBlockTemplate(size_t max_transactions) {
    // Get current chain height
    uint32_t height = static_cast<uint32_t>(chainstate_.GetHeight() + 1);

    // Get initial difficulty bits
    uint32_t bits = consensus::Difficulty::GetInitialBits();

    // Convert bits to target for template
    uint256_t target = consensus::Difficulty::CompactToBits256(bits);

    // Select transactions from mempool
    auto transactions = SelectTransactions(max_transactions);

    // Calculate fees from selected transactions
    auto fees = CalculateFees(transactions);

    // Calculate coinbase rewards (block reward + fees)
    std::vector<primitives::AssetAmount> coinbase_rewards;

    // Get block rewards for each asset
    auto tal_reward = consensus::Issuance::GetBlockReward(height, primitives::AssetID::TALANTON);
    auto dra_reward = consensus::Issuance::GetBlockReward(height, primitives::AssetID::DRACHMA);
    auto obl_reward = consensus::Issuance::GetBlockReward(height, primitives::AssetID::OBOLOS);

    // Add fees to rewards
    coinbase_rewards.push_back(primitives::AssetAmount(
        primitives::AssetID::TALANTON, tal_reward + fees[primitives::AssetID::TALANTON]));
    coinbase_rewards.push_back(primitives::AssetAmount(
        primitives::AssetID::DRACHMA, dra_reward + fees[primitives::AssetID::DRACHMA]));
    coinbase_rewards.push_back(primitives::AssetAmount(
        primitives::AssetID::OBOLOS, obl_reward + fees[primitives::AssetID::OBOLOS]));

    // Create coinbase transaction
    auto coinbase = CreateCoinbaseTransaction(height, coinbase_rewards);

    // Construct block
    BlockTemplate block_template;
    block_template.block.header.version = 1;
    // Previous block hash would come from chain tip - use zeros for now
    block_template.block.header.prev_block_hash = std::array<uint8_t, 32>{};
    block_template.block.header.timestamp = static_cast<uint32_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    block_template.block.header.nonce = 0;
    block_template.block.header.bits = bits;  // Use calculated difficulty bits

    // Add transactions (coinbase first)
    block_template.block.transactions.push_back(coinbase);
    block_template.block.transactions.insert(block_template.block.transactions.end(),
                                             transactions.begin(), transactions.end());

    // Calculate merkle root
    block_template.block.header.merkle_root = ComputeMerkleRoot(block_template.block.transactions);

    // Fill template metadata
    block_template.height = height;
    block_template.target = target;
    block_template.coinbase_rewards = coinbase_rewards;

    uint64_t total_fees_sum = 0;
    for (const auto& [asset, fee] : fees) {
        total_fees_sum += fee;
    }
    block_template.total_fees = total_fees_sum;

    return block_template;
}

std::optional<primitives::Block> Miner::MineBlock(const BlockTemplate& block_template,
                                                  uint64_t max_iterations) {
    is_mining_ = true;
    total_hashes_ = 0;

    primitives::Block block = block_template.block;
    uint256_t target = block_template.target;

    auto start_time = std::chrono::steady_clock::now();

    // Mining loop - try different nonces
    for (uint64_t i = 0; max_iterations == 0 || i < max_iterations; i++) {
        if (!is_mining_) {
            return std::nullopt;  // Mining stopped
        }

        block.header.nonce = static_cast<uint32_t>(i);

        // Check if this nonce produces a valid block
        if (VerifyProofOfWork(block, target)) {
            // Found valid block!
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

            if (duration.count() > 0) {
                hashrate_ = total_hashes_ / duration.count();
            }

            is_mining_ = false;
            return block;
        }

        total_hashes_++;

        // Update hashrate periodically
        if (i % 100000 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
            if (duration.count() > 0) {
                hashrate_ = total_hashes_ / duration.count();
            }
        }
    }

    is_mining_ = false;
    return std::nullopt;  // Max iterations reached without finding block
}

bool Miner::VerifyProofOfWork(const primitives::Block& block, const uint256_t& target) {
    // Get block hash (double SHA-256)
    auto block_hash = block.GetHash();

    // Check if hash < target (big-endian comparison)
    // Note: uint256_t is std::array<uint8_t, 32> from EVM state.h
    for (size_t i = 0; i < 32; i++) {
        if (block_hash[i] < static_cast<uint8_t>(target[i])) {
            return true;  // Hash is less than target
        } else if (block_hash[i] > static_cast<uint8_t>(target[i])) {
            return false;  // Hash is greater than target
        }
    }

    return false;  // Hash equals target (also valid, but extremely rare)
}

MiningStatus Miner::GetStatus() const {
    MiningStatus status;
    status.is_mining = is_mining_;
    status.height = static_cast<uint32_t>(chainstate_.GetHeight() + 1);
    status.hashrate = hashrate_;
    status.total_hashes = total_hashes_;
    status.current_block_hash = std::array<uint8_t, 32>{};

    return status;
}

void Miner::StopMining() {
    is_mining_ = false;
}

primitives::Transaction
Miner::CreateCoinbaseTransaction(uint32_t height,
                                 const std::vector<primitives::AssetAmount>& rewards) {
    primitives::Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;

    // Coinbase input (special marker)
    primitives::TxInput coinbase_input;
    coinbase_input.prevout.txid = std::array<uint8_t, 32>{};
    coinbase_input.prevout.vout = primitives::COINBASE_VOUT_INDEX;
    coinbase_input.sequence = 0xFFFFFFFF;

    // Coinbase signature script contains height (BIP-34 style)
    coinbase_input.signature_script.push_back(static_cast<uint8_t>(height));
    coinbase_input.signature_script.push_back(static_cast<uint8_t>(height >> 8));
    coinbase_input.signature_script.push_back(static_cast<uint8_t>(height >> 16));
    coinbase_input.signature_script.push_back(static_cast<uint8_t>(height >> 24));

    coinbase.inputs.push_back(coinbase_input);

    // Coinbase outputs (one per asset with non-zero reward)
    for (const auto& reward : rewards) {
        if (reward.amount > 0) {
            primitives::TxOutput output;
            output.value = reward;
            output.pubkey_script = coinbase_pubkey_;
            coinbase.outputs.push_back(output);
        }
    }

    return coinbase;
}

std::map<primitives::AssetID, uint64_t>
Miner::CalculateFees(const std::vector<primitives::Transaction>& transactions) {
    std::map<primitives::AssetID, uint64_t> fees;

    // Initialize fees to zero
    fees[primitives::AssetID::TALANTON] = 0;
    fees[primitives::AssetID::DRACHMA] = 0;
    fees[primitives::AssetID::OBOLOS] = 0;

    // Sum output values grouped by AssetID.
    // True fee = sum(inputs) - sum(outputs), but UTXO lookups are not available
    // in the current ChainState interface, so output sums serve as a conservative proxy.
    for (const auto& tx : transactions) {
        for (const auto& output : tx.outputs) {
            fees[output.value.asset] += output.value.amount;
        }
    }

    return fees;
}

std::vector<primitives::Transaction> Miner::SelectTransactions(size_t max_count) {
    std::vector<primitives::Transaction> selected;

    // No mempool access in current interface; return empty list
    (void)max_count;

    return selected;
}

std::array<uint8_t, 32>
Miner::ComputeMerkleRoot(const std::vector<primitives::Transaction>& transactions) {
    if (transactions.empty()) {
        return std::array<uint8_t, 32>{};
    }

    // Build Merkle tree bottom-up
    std::vector<std::array<uint8_t, 32>> hashes;

    // Hash all transactions
    for (const auto& tx : transactions) {
        hashes.push_back(tx.GetTxID());
    }

    // Build tree by hashing pairs
    while (hashes.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next_level;

        for (size_t i = 0; i < hashes.size(); i += 2) {
            crypto::SHA256 hasher;

            // Hash first element
            hasher.Write(hashes[i].data(), 32);

            // Hash second element (or duplicate if odd number)
            if (i + 1 < hashes.size()) {
                hasher.Write(hashes[i + 1].data(), 32);
            } else {
                hasher.Write(hashes[i].data(), 32);
            }

            auto combined_hash = hasher.Finalize();
            std::array<uint8_t, 32> result;
            // Safe memcpy - both are 32 bytes
            static_assert(sizeof(result) == sizeof(combined_hash), "Hash sizes must match");
            std::memcpy(result.data(), combined_hash.data(), 32);
            next_level.push_back(result);
        }

        hashes = next_level;
    }

    return hashes[0];
}

}  // namespace mining
}  // namespace parthenon
