// ParthenonChain - Transaction Primitives
// Consensus-critical: UTXO-based transaction model with multi-asset support

#ifndef PARTHENON_PRIMITIVES_TRANSACTION_H
#define PARTHENON_PRIMITIVES_TRANSACTION_H

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include "amount.h"
#include "asset.h"

#include <array>
#include <optional>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace primitives {

// Coinbase transaction marker
static constexpr uint32_t COINBASE_VOUT_INDEX = 0xFFFFFFFF;

/**
 * OutPoint identifies a specific output from a previous transaction
 * (txid, output_index)
 */
struct OutPoint {
    std::array<uint8_t, 32> txid;  // Transaction ID (SHA-256d hash)
    uint32_t vout;                 // Output index

    OutPoint() : txid{}, vout(0) {}
    OutPoint(const std::array<uint8_t, 32>& tx, uint32_t v) : txid(tx), vout(v) {}

    bool operator==(const OutPoint& other) const {
        return txid == other.txid && vout == other.vout;
    }

    bool operator!=(const OutPoint& other) const { return !(*this == other); }

    bool operator<(const OutPoint& other) const {
        if (txid != other.txid) {
            return txid < other.txid;
        }
        return vout < other.vout;
    }

    // Serialize to bytes (32 bytes txid + 4 bytes vout)
    void Serialize(std::vector<uint8_t>& output) const;

    // Deserialize from bytes
    static OutPoint Deserialize(const uint8_t* input);
};

/**
 * TxInput represents a transaction input (spending a previous output)
 */
struct TxInput {
    OutPoint prevout;                       // Previous output being spent
    std::vector<uint8_t> signature_script;  // Schnorr signature + public key
    uint32_t sequence;                      // Sequence number (for timelocks, RBF)

    TxInput() : sequence(0xFFFFFFFF) {}

    bool operator==(const TxInput& other) const {
        return prevout == other.prevout && signature_script == other.signature_script &&
               sequence == other.sequence;
    }

    // Serialize input
    void Serialize(std::vector<uint8_t>& output) const;

    // Deserialize input
    static TxInput Deserialize(const uint8_t*& input);
};

/**
 * TxOutput represents a transaction output
 * Each output has an asset type and amount, plus a locking script (pubkey)
 */
struct TxOutput {
    AssetAmount value;                   // Asset and amount
    std::vector<uint8_t> pubkey_script;  // Locking script (x-only pubkey for Schnorr)

    TxOutput() = default;
    TxOutput(AssetID asset, uint64_t amount, const std::vector<uint8_t>& script)
        : value(asset, amount), pubkey_script(script) {}

    bool operator==(const TxOutput& other) const {
        return value == other.value && pubkey_script == other.pubkey_script;
    }

    // Validate output (check amount doesn't exceed asset max supply)
    bool IsValid() const { return value.IsValid() && !pubkey_script.empty(); }

    // Serialize output
    void Serialize(std::vector<uint8_t>& output) const;

    // Deserialize output
    static TxOutput Deserialize(const uint8_t*& input);
};

/**
 * Transaction represents a complete transaction
 * Multi-asset UTXO model
 */
class Transaction {
  public:
    uint32_t version;               // Transaction version
    std::vector<TxInput> inputs;    // Transaction inputs
    std::vector<TxOutput> outputs;  // Transaction outputs
    uint32_t locktime;              // Locktime (0 = no locktime)

    Transaction() : version(1), locktime(0) {}

    /**
     * Get transaction ID (SHA-256d of serialized transaction)
     * This is the canonical transaction identifier
     */
    std::array<uint8_t, 32> GetTxID() const;

    /**
     * Get transaction hash for signing (excludes signatures)
     * Used during signature verification
     */
    std::array<uint8_t, 32> GetSignatureHash(size_t input_index) const;

    /**
     * Serialize transaction to bytes
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * Deserialize transaction from bytes
     */
    static std::optional<Transaction> Deserialize(const uint8_t* data, size_t len);

    /**
     * Validate transaction structure
     * - Must have at least one input and one output
     * - All outputs must be valid
     * - No duplicate inputs
     * - Asset conservation per asset type
     */
    bool IsValid() const;

    /**
     * Check if this is a coinbase transaction (mining reward)
     */
    bool IsCoinbase() const {
        return inputs.size() == 1 && inputs[0].prevout.vout == COINBASE_VOUT_INDEX &&
               inputs[0].prevout.txid == std::array<uint8_t, 32>{};
    }

  private:
    /**
     * Serialize for signing (excludes input signatures)
     */
    std::vector<uint8_t> SerializeForSigning(size_t input_index) const;
};

/**
 * Helper: Write compact size (variable-length integer encoding)
 */
void WriteCompactSize(std::vector<uint8_t>& output, uint64_t size);

/**
 * Helper: Read compact size
 */
uint64_t ReadCompactSize(const uint8_t*& input);

}  // namespace primitives
}  // namespace parthenon

#endif  // PARTHENON_PRIMITIVES_TRANSACTION_H
