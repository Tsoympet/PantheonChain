// ParthenonChain - UTXO Storage Module
// LevelDB-based persistent UTXO set storage

#pragma once

#include "chainstate/utxo.h"
#include "primitives/transaction.h"

#include <leveldb/db.h>
#include <memory>
#include <optional>
#include <string>

namespace parthenon {
namespace storage {

/**
 * UTXOStorage provides persistent storage for UTXO set using LevelDB
 *
 * Storage layout:
 * - "u{txid}{vout}" -> serialized TxOutput
 * - "meta:utxo_count" -> total number of UTXOs
 */
class UTXOStorage {
  public:
    /**
     * Open UTXO storage database
     * @param db_path Path to LevelDB database directory
     * @return true if opened successfully
     */
    bool Open(const std::string& db_path);

    /**
     * Close the database
     */
    void Close();

    /**
     * Add a UTXO to storage
     * @param txid Transaction ID
     * @param vout Output index
     * @param output Transaction output
     * @return true if stored successfully
     */
    bool AddUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout,
                 const primitives::TxOutput& output);

    /**
     * Remove a UTXO from storage (spent)
     * @param txid Transaction ID
     * @param vout Output index
     * @return true if removed successfully
     */
    bool RemoveUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout);

    /**
     * Get a UTXO from storage
     * @param txid Transaction ID
     * @param vout Output index
     * @return Output if found, nullopt otherwise
     */
    std::optional<primitives::TxOutput> GetUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout);

    /**
     * Check if a UTXO exists
     * @param txid Transaction ID
     * @param vout Output index
     * @return true if UTXO exists
     */
    bool HasUTXO(const std::array<uint8_t, 32>& txid, uint32_t vout);

    /**
     * Load entire UTXO set into memory
     * @param utxo_set UTXO set to populate
     * @return true if loaded successfully
     */
    bool LoadUTXOSet(chainstate::UTXOSet& utxo_set);

    /**
     * Save entire UTXO set to disk
     * @param utxo_set UTXO set to save
     * @return true if saved successfully
     */
    bool SaveUTXOSet(const chainstate::UTXOSet& utxo_set);

    /**
     * Get total number of UTXOs
     */
    uint64_t GetUTXOCount();

    /**
     * Check if database is open
     */
    bool IsOpen() const { return db_ != nullptr; }

  private:
    std::unique_ptr<leveldb::DB> db_;

    // Helper functions
    std::string UTXOKey(const std::array<uint8_t, 32>& txid, uint32_t vout);
    std::string SerializeOutput(const primitives::TxOutput& output);
    std::optional<primitives::TxOutput> DeserializeOutput(const std::string& data);
};

}  // namespace storage
}  // namespace parthenon
