// ParthenonChain - UTXO Storage Implementation

#include "utxo_storage.h"

#include <charconv>
#include <cctype>
#include <leveldb/write_batch.h>
#include <sstream>

namespace parthenon {
namespace storage {

namespace {

bool TryParseUint32(const std::string& value, uint32_t& out) {
    if (value.empty()) {
        return false;
    }

    uint32_t parsed = 0;
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (ec != std::errc{} || ptr != value.data() + value.size()) {
        return false;
    }

    out = parsed;
    return true;
}

bool TryParseUint64(const std::string& value, uint64_t& out) {
    if (value.empty()) {
        return false;
    }

    uint64_t parsed = 0;
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (ec != std::errc{} || ptr != value.data() + value.size()) {
        return false;
    }

    out = parsed;
    return true;
}

bool TryParseHexByte(const char high, const char low, uint8_t& out) {
    auto to_nibble = [](const char c, uint8_t& nibble) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isdigit(uc)) {
            nibble = static_cast<uint8_t>(c - '0');
            return true;
        }
        if (c >= 'a' && c <= 'f') {
            nibble = static_cast<uint8_t>(10 + (c - 'a'));
            return true;
        }
        if (c >= 'A' && c <= 'F') {
            nibble = static_cast<uint8_t>(10 + (c - 'A'));
            return true;
        }
        return false;
    };

    uint8_t hi = 0;
    uint8_t lo = 0;
    if (!to_nibble(high, hi) || !to_nibble(low, lo)) {
        return false;
    }

    out = static_cast<uint8_t>((hi << 4) | lo);
    return true;
}

}  // namespace

bool UTXOStorage::Open(const std::string& db_path) {
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::DB* db_ptr;
    leveldb::Status status = leveldb::DB::Open(options, db_path, &db_ptr);

    if (!status.ok()) {
        return false;
    }

    db_.reset(db_ptr);
    return true;
}

void UTXOStorage::Close() {
    db_.reset();
}

std::string UTXOStorage::UTXOKey(const std::array<uint8_t, 32>& txid, uint32_t vout) {
    std::string key = "u";
    for (uint8_t byte : txid) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", byte);
        key += buf;
    }
    key += "_";
    key += std::to_string(vout);
    return key;
}

std::string UTXOStorage::SerializeOutput(const primitives::TxOutput& output) {
    std::ostringstream oss;

    // Serialize asset amount
    uint8_t asset_id = static_cast<uint8_t>(output.value.asset);
    oss.write(reinterpret_cast<const char*>(&asset_id), sizeof(asset_id));
    oss.write(reinterpret_cast<const char*>(&output.value.amount), sizeof(output.value.amount));

    // Serialize pubkey_script
    uint32_t script_size = static_cast<uint32_t>(output.pubkey_script.size());
    oss.write(reinterpret_cast<const char*>(&script_size), sizeof(script_size));
    oss.write(reinterpret_cast<const char*>(output.pubkey_script.data()), script_size);

    return oss.str();
}

std::optional<primitives::TxOutput> UTXOStorage::DeserializeOutput(const std::string& data) {
    if (data.size() < sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint32_t)) {
        return std::nullopt;
    }

    std::istringstream iss(data);
    primitives::TxOutput output;

    // Deserialize asset amount
    uint8_t asset_id;
    iss.read(reinterpret_cast<char*>(&asset_id), sizeof(asset_id));
    output.value.asset = static_cast<primitives::AssetID>(asset_id);
    iss.read(reinterpret_cast<char*>(&output.value.amount), sizeof(output.value.amount));

    // Deserialize pubkey_script
    uint32_t script_size;
    iss.read(reinterpret_cast<char*>(&script_size), sizeof(script_size));
    output.pubkey_script.resize(script_size);
    iss.read(reinterpret_cast<char*>(output.pubkey_script.data()), script_size);

    return output;
}

bool UTXOStorage::AddUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout,
                          const primitives::TxOutput& output) {
    if (!db_) {
        return false;
    }

    std::string key = UTXOKey(txid, vout);
    std::string value = SerializeOutput(output);

    leveldb::WriteOptions options;
    leveldb::Status status = db_->Put(options, key, value);

    if (status.ok()) {
        // Update count
        uint64_t count = GetUTXOCount();
        db_->Put(options, "meta:utxo_count", std::to_string(count + 1));
    }

    return status.ok();
}

bool UTXOStorage::RemoveUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout) {
    if (!db_) {
        return false;
    }

    std::string key = UTXOKey(txid, vout);

    leveldb::WriteOptions options;
    leveldb::Status status = db_->Delete(options, key);

    if (status.ok()) {
        // Update count
        uint64_t count = GetUTXOCount();
        if (count > 0) {
            db_->Put(options, "meta:utxo_count", std::to_string(count - 1));
        }
    }

    return status.ok();
}

std::optional<primitives::TxOutput> UTXOStorage::GetUTXO(const std::array<uint8_t, 32>& txid,
                                                         uint32_t vout) {
    if (!db_) {
        return std::nullopt;
    }

    std::string key = UTXOKey(txid, vout);
    std::string value;

    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    if (!status.ok()) {
        return std::nullopt;
    }

    return DeserializeOutput(value);
}

bool UTXOStorage::HasUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout) {
    return GetUTXO(txid, vout).has_value();
}

bool UTXOStorage::LoadUTXOSet(chainstate::UTXOSet& utxo_set) {
    if (!db_) {
        return false;
    }

    // Iterate over all UTXOs
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions()));

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();

        // Only process UTXO keys (starting with 'u')
        if (key.empty() || key[0] != 'u') {
            continue;
        }

        // Parse txid and vout from key
        // Key format: "u{64_hex_chars}_{vout}"
        if (key.length() < 66) {  // "u" + 64 hex chars + "_" + digit
            continue;
        }

        // Extract txid (skip 'u' prefix)
        const size_t underscore_pos = key.find('_', 65);
        if (underscore_pos == std::string::npos || underscore_pos <= 64) {
            continue;
        }

        std::array<uint8_t, 32> txid{};
        bool txid_valid = true;
        for (size_t i = 0; i < 32; i++) {
            uint8_t byte = 0;
            if (!TryParseHexByte(key[1 + i * 2], key[1 + i * 2 + 1], byte)) {
                txid_valid = false;
                break;
            }
            txid[i] = byte;
        }
        if (!txid_valid) {
            continue;
        }

        // Extract vout (after underscore)
        uint32_t vout = 0;
        if (!TryParseUint32(key.substr(underscore_pos + 1), vout)) {
            continue;
        }

        // Deserialize output
        auto output = DeserializeOutput(it->value().ToString());
        if (output.has_value()) {
            // Create a Coin from the output (height and is_coinbase unknown from storage)
            chainstate::Coin coin(output.value(), 0, false);
            primitives::OutPoint outpoint(txid, vout);
            utxo_set.AddCoin(outpoint, coin);
        }
    }

    return it->status().ok();
}

bool UTXOStorage::SaveUTXOSet(const chainstate::UTXOSet& utxo_set) {
    if (!db_) {
        return false;
    }

    // Clear existing UTXOs
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions()));
    leveldb::WriteBatch batch;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (!key.empty() && key[0] == 'u') {
            batch.Delete(key);
        }
    }

    // Add current UTXOs
    auto& utxos = utxo_set.GetUTXOs();
    uint64_t count = 0;

    for (const auto& [outpoint, coin] : utxos) {
        std::string key = UTXOKey(outpoint.txid, outpoint.vout);
        std::string value = SerializeOutput(coin.output);
        batch.Put(key, value);
        count++;
    }

    // Update count
    batch.Put("meta:utxo_count", std::to_string(count));

    leveldb::WriteOptions options;
    leveldb::Status status = db_->Write(options, &batch);

    return status.ok();
}

uint64_t UTXOStorage::GetUTXOCount() {
    if (!db_) {
        return 0;
    }

    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), "meta:utxo_count", &value);

    if (!status.ok()) {
        return 0;
    }

    uint64_t count = 0;
    if (!TryParseUint64(value, count)) {
        return 0;
    }

    return count;
}

}  // namespace storage
}  // namespace parthenon
