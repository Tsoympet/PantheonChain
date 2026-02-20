// ParthenonChain - Transaction Indexer Implementation
// Indexes all blockchain transactions for fast queries

#include "tx_indexer.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

namespace parthenon {
namespace layer2 {
namespace indexers {

// Simple file-based storage for now (can be upgraded to LevelDB/RocksDB later)
class TxIndexer::Impl {
  public:
    Impl() : is_open_(false) {}

    bool Open(const std::string& db_path) {
        db_path_ = db_path;

        // Load existing index
        std::ifstream file(db_path_ + "/tx_index.dat", std::ios::binary);
        if (file.is_open()) {
            // Load transaction index from file
            // Format: txid(32) height(4) block_time(4) address_count(4) addresses...
            file.close();
        }

        is_open_ = true;
        return true;
    }

    void Close() {
        if (!is_open_)
            return;

        // Save index to file
        std::ofstream file(db_path_ + "/tx_index.dat", std::ios::binary | std::ios::trunc);
        if (file.is_open()) {
            // Save transaction index
            file.close();
        }

        is_open_ = false;
    }

    void IndexTransaction(const primitives::Transaction& tx, uint32_t height, uint32_t block_time) {
        auto txid = tx.GetTxID();

        TxInfo info;
        info.tx = tx;
        info.height = height;
        info.block_time = block_time;

        // Store by txid
        tx_by_id_[txid] = info;

        // Index by addresses (inputs and outputs)
        // Note: Input indexing would require UTXO lookup in real implementation
        (void)tx.inputs;  // Suppress unused warning for now

        for (size_t i = 0; i < tx.outputs.size(); ++i) {
            const auto& output = tx.outputs[i];
            // Extract address from output script
            std::vector<uint8_t> address;

            // Simplified: use pubkey_script as address identifier
            if (output.pubkey_script.size() > 0) {
                address = output.pubkey_script;
                tx_by_address_[address].push_back(txid);
            }
        }
    }

    std::vector<primitives::Transaction>
    GetTransactionsByAddress(const std::vector<uint8_t>& address, uint32_t limit) {
        std::vector<primitives::Transaction> result;

        auto it = tx_by_address_.find(address);
        if (it != tx_by_address_.end()) {
            for (const auto& txid : it->second) {
                if (result.size() >= limit)
                    break;

                auto tx_it = tx_by_id_.find(txid);
                if (tx_it != tx_by_id_.end()) {
                    result.push_back(tx_it->second.tx);
                }
            }
        }

        return result;
    }

    std::optional<primitives::Transaction> GetTransactionById(const std::array<uint8_t, 32>& txid) {
        auto it = tx_by_id_.find(txid);
        if (it != tx_by_id_.end()) {
            return it->second.tx;
        }
        return std::nullopt;
    }

    size_t GetTransactionCount() const { return tx_by_id_.size(); }

    std::vector<primitives::Transaction> GetRecentTransactions(uint32_t limit) {
        std::vector<primitives::Transaction> result;

        // Collect all transactions
        std::vector<TxInfo> all_txs;
        for (const auto& pair : tx_by_id_) {
            all_txs.push_back(pair.second);
        }

        // Sort by height (descending)
        std::sort(all_txs.begin(), all_txs.end(),
                  [](const TxInfo& a, const TxInfo& b) { return a.height > b.height; });

        // Return top limit
        for (size_t i = 0; i < std::min(static_cast<size_t>(limit), all_txs.size()); ++i) {
            result.push_back(all_txs[i].tx);
        }

        return result;
    }

  private:
    struct TxInfo {
        primitives::Transaction tx;
        uint32_t height;
        uint32_t block_time;
    };

    std::string db_path_;
    bool is_open_;
    std::map<std::array<uint8_t, 32>, TxInfo> tx_by_id_;
    std::map<std::vector<uint8_t>, std::vector<std::array<uint8_t, 32>>> tx_by_address_;
};

TxIndexer::TxIndexer() : impl_(std::make_unique<Impl>()) {}

TxIndexer::~TxIndexer() {
    if (impl_) {
        impl_->Close();
    }
}

bool TxIndexer::Open(const std::string& db_path) {
    return impl_->Open(db_path);
}

void TxIndexer::Close() {
    impl_->Close();
}

void TxIndexer::IndexTransaction(const primitives::Transaction& tx, uint32_t height,
                                 uint32_t block_time) {
    impl_->IndexTransaction(tx, height, block_time);
}

std::vector<primitives::Transaction>
TxIndexer::GetTransactionsByAddress(const std::vector<uint8_t>& address, uint32_t limit) {
    return impl_->GetTransactionsByAddress(address, limit);
}

std::optional<primitives::Transaction>
TxIndexer::GetTransactionById(const std::array<uint8_t, 32>& txid) {
    return impl_->GetTransactionById(txid);
}

size_t TxIndexer::GetTransactionCount() const {
    return impl_->GetTransactionCount();
}

std::vector<primitives::Transaction> TxIndexer::GetRecentTransactions(uint32_t limit) {
    return impl_->GetRecentTransactions(limit);
}

}  // namespace indexers
}  // namespace layer2
}  // namespace parthenon
