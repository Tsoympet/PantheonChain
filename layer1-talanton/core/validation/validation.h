// ParthenonChain - Validation Module
// Consensus-critical: Transaction and block validation

#ifndef PARTHENON_VALIDATION_VALIDATION_H
#define PARTHENON_VALIDATION_VALIDATION_H

#include "chainstate/utxo.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

#include <map>
#include <optional>
#include <string>

namespace parthenon {
namespace validation {

/**
 * ValidationError describes why validation failed
 */
struct ValidationError {
    enum class Type {
        // Transaction errors
        TX_NO_INPUTS,
        TX_NO_OUTPUTS,
        TX_DUPLICATE_INPUTS,
        TX_MISSING_INPUT,
        TX_IMMATURE_COINBASE,
        TX_INVALID_SIGNATURE,
        TX_ASSET_CONSERVATION,
        TX_INVALID_AMOUNT,

        // Block errors
        BLOCK_NO_TRANSACTIONS,
        BLOCK_NO_COINBASE,
        BLOCK_MULTIPLE_COINBASE,
        BLOCK_INVALID_MERKLE_ROOT,
        BLOCK_INVALID_POW,
        BLOCK_INVALID_COINBASE_REWARD,
        BLOCK_EXCEEDS_SUPPLY_CAP,
        BLOCK_INVALID_TRANSACTION,

        // Other
        UNKNOWN
    };

    Type type;
    std::string message;

    ValidationError(Type t, const std::string& msg) : type(t), message(msg) {}
};

/**
 * TransactionValidator validates individual transactions
 */
class TransactionValidator {
  public:
    /**
     * Validate transaction structure
     * - Must have inputs and outputs
     * - No duplicate inputs
     * - Valid amounts
     */
    static std::optional<ValidationError> ValidateStructure(const primitives::Transaction& tx);

    /**
     * Validate transaction against UTXO set
     * - All inputs must exist
     * - Coinbase maturity enforced
     * - Asset conservation enforced
     *
     * @param tx Transaction to validate
     * @param utxo_set UTXO set to validate against
     * @param height Current block height (for coinbase maturity)
     */
    static std::optional<ValidationError> ValidateAgainstUTXO(const primitives::Transaction& tx,
                                                              const chainstate::UTXOSet& utxo_set,
                                                              uint32_t height);

    /**
     * Validate transaction signatures
     * Verifies Schnorr signatures (BIP-340) for all transaction inputs
     *
     * @param tx Transaction to validate
     * @param utxo_set UTXO set containing outputs being spent (to get public keys)
     * @return ValidationError if any signature is invalid, std::nullopt if all valid
     */
    static std::optional<ValidationError> ValidateSignatures(const primitives::Transaction& tx,
                                                             const chainstate::UTXOSet& utxo_set);
};

/**
 * BlockValidator validates blocks
 */
class BlockValidator {
  public:
    /**
     * Validate block structure
     * - Must have at least one transaction (coinbase)
     * - First transaction must be coinbase
     * - Only first transaction can be coinbase
     * - Merkle root must match
     */
    static std::optional<ValidationError> ValidateStructure(const primitives::Block& block);

    /**
     * Validate proof-of-work
     */
    static std::optional<ValidationError> ValidatePoW(const primitives::Block& block);

    /**
     * Validate coinbase transaction
     * - Rewards must not exceed allowed amounts
     * - Must respect supply caps
     *
     * @param block Block containing coinbase
     * @param height Block height
     * @param current_supply Current supply per asset
     */
    static std::optional<ValidationError>
    ValidateCoinbase(const primitives::Block& block, uint32_t height,
                     const std::map<primitives::AssetID, uint64_t>& current_supply);

    /**
     * Full block validation
     * Combines all validation checks
     */
    static std::optional<ValidationError>
    ValidateBlock(const primitives::Block& block, const chainstate::UTXOSet& utxo_set,
                  uint32_t height, const std::map<primitives::AssetID, uint64_t>& current_supply);
};

}  // namespace validation
}  // namespace parthenon

#endif  // PARTHENON_VALIDATION_VALIDATION_H
