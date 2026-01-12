// ParthenonChain - Validation Implementation
// Consensus-critical: Must be deterministic

#include "validation.h"
#include "consensus/issuance.h"
#include "consensus/difficulty.h"
#include <set>

namespace parthenon {
namespace validation {

// Transaction Validation

std::optional<ValidationError> TransactionValidator::ValidateStructure(
    const primitives::Transaction& tx
) {
    // Coinbase can have zero inputs
    if (!tx.IsCoinbase()) {
        if (tx.inputs.empty()) {
            return ValidationError(
                ValidationError::Type::TX_NO_INPUTS,
                "Transaction has no inputs"
            );
        }
    }
    
    if (tx.outputs.empty()) {
        return ValidationError(
            ValidationError::Type::TX_NO_OUTPUTS,
            "Transaction has no outputs"
        );
    }
    
    // Check for duplicate inputs
    std::set<primitives::OutPoint> seen_inputs;
    for (const auto& input : tx.inputs) {
        if (seen_inputs.count(input.prevout) > 0) {
            return ValidationError(
                ValidationError::Type::TX_DUPLICATE_INPUTS,
                "Transaction has duplicate inputs"
            );
        }
        seen_inputs.insert(input.prevout);
    }
    
    // Validate output amounts
    for (const auto& output : tx.outputs) {
        if (!output.value.IsValid()) {
            return ValidationError(
                ValidationError::Type::TX_INVALID_AMOUNT,
                "Transaction output has invalid amount"
            );
        }
    }
    
    return std::nullopt; // Valid
}

std::optional<ValidationError> TransactionValidator::ValidateAgainstUTXO(
    const primitives::Transaction& tx,
    const chainstate::UTXOSet& utxo_set,
    uint32_t height
) {
    // Coinbase doesn't spend inputs
    if (tx.IsCoinbase()) {
        return std::nullopt;
    }
    
    // Track inputs and outputs by asset
    std::map<primitives::AssetID, uint64_t> input_amounts;
    std::map<primitives::AssetID, uint64_t> output_amounts;
    
    // Check all inputs exist and are spendable
    for (const auto& input : tx.inputs) {
        auto coin = utxo_set.GetCoin(input.prevout);
        if (!coin) {
            return ValidationError(
                ValidationError::Type::TX_MISSING_INPUT,
                "Transaction input does not exist in UTXO set"
            );
        }
        
        // Check coinbase maturity
        if (!coin->IsSpendable(height)) {
            return ValidationError(
                ValidationError::Type::TX_IMMATURE_COINBASE,
                "Transaction spends immature coinbase output"
            );
        }
        
        // Accumulate input amounts
        auto asset = coin->output.value.asset;
        input_amounts[asset] += coin->output.value.amount;
    }
    
    // Accumulate output amounts
    for (const auto& output : tx.outputs) {
        auto asset = output.value.asset;
        output_amounts[asset] += output.value.amount;
    }
    
    // Check asset conservation
    for (const auto& [asset, out_amount] : output_amounts) {
        uint64_t in_amount = input_amounts[asset];
        if (in_amount < out_amount) {
            return ValidationError(
                ValidationError::Type::TX_ASSET_CONSERVATION,
                "Transaction creates assets from thin air"
            );
        }
    }
    
    return std::nullopt; // Valid
}

std::optional<ValidationError> TransactionValidator::ValidateSignatures(
    const primitives::Transaction& tx,
    const chainstate::UTXOSet& utxo_set
) {
    // TODO: Implement signature validation
    // For now, accept all transactions
    // Will be implemented when full cryptographic validation is added
    (void)tx;
    (void)utxo_set;
    return std::nullopt;
}

// Block Validation

std::optional<ValidationError> BlockValidator::ValidateStructure(
    const primitives::Block& block
) {
    // Must have at least one transaction
    if (block.transactions.empty()) {
        return ValidationError(
            ValidationError::Type::BLOCK_NO_TRANSACTIONS,
            "Block has no transactions"
        );
    }
    
    // First transaction must be coinbase
    if (!block.transactions[0].IsCoinbase()) {
        return ValidationError(
            ValidationError::Type::BLOCK_NO_COINBASE,
            "Block's first transaction is not coinbase"
        );
    }
    
    // Only first transaction can be coinbase
    for (size_t i = 1; i < block.transactions.size(); i++) {
        if (block.transactions[i].IsCoinbase()) {
            return ValidationError(
                ValidationError::Type::BLOCK_MULTIPLE_COINBASE,
                "Block has multiple coinbase transactions"
            );
        }
    }
    
    // Validate merkle root
    auto calculated_root = block.CalculateMerkleRoot();
    if (calculated_root != block.header.merkle_root) {
        return ValidationError(
            ValidationError::Type::BLOCK_INVALID_MERKLE_ROOT,
            "Block merkle root does not match calculated value"
        );
    }
    
    return std::nullopt; // Valid
}

std::optional<ValidationError> BlockValidator::ValidatePoW(
    const primitives::Block& block
) {
    auto hash = block.GetHash();
    if (!consensus::Difficulty::CheckProofOfWork(hash, block.header.bits)) {
        return ValidationError(
            ValidationError::Type::BLOCK_INVALID_POW,
            "Block does not meet difficulty target"
        );
    }
    
    return std::nullopt; // Valid
}

std::optional<ValidationError> BlockValidator::ValidateCoinbase(
    const primitives::Block& block,
    uint32_t height,
    const std::map<primitives::AssetID, uint64_t>& current_supply
) {
    const auto& coinbase = block.transactions[0];
    
    // Sum coinbase outputs by asset
    std::map<primitives::AssetID, uint64_t> coinbase_amounts;
    for (const auto& output : coinbase.outputs) {
        coinbase_amounts[output.value.asset] += output.value.amount;
    }
    
    // Validate each asset's reward
    for (const auto& [asset, amount] : coinbase_amounts) {
        // Check reward doesn't exceed allowed amount
        if (!consensus::Issuance::IsValidBlockReward(height, asset, amount)) {
            return ValidationError(
                ValidationError::Type::BLOCK_INVALID_COINBASE_REWARD,
                "Block coinbase reward exceeds allowed amount"
            );
        }
        
        // Check won't exceed supply cap
        auto it = current_supply.find(asset);
        uint64_t supply = (it != current_supply.end()) ? it->second : 0;
        
        // Check for overflow
        if (supply + amount < supply) {
            return ValidationError(
                ValidationError::Type::BLOCK_EXCEEDS_SUPPLY_CAP,
                "Block coinbase would cause supply overflow"
            );
        }
        
        uint64_t new_supply = supply + amount;
        if (new_supply > primitives::AssetSupply::GetMaxSupply(asset)) {
            return ValidationError(
                ValidationError::Type::BLOCK_EXCEEDS_SUPPLY_CAP,
                "Block coinbase would exceed supply cap"
            );
        }
    }
    
    return std::nullopt; // Valid
}

std::optional<ValidationError> BlockValidator::ValidateBlock(
    const primitives::Block& block,
    const chainstate::UTXOSet& utxo_set,
    uint32_t height,
    const std::map<primitives::AssetID, uint64_t>& current_supply
) {
    // Validate structure
    auto error = ValidateStructure(block);
    if (error) return error;
    
    // Validate PoW
    error = ValidatePoW(block);
    if (error) return error;
    
    // Validate coinbase
    error = ValidateCoinbase(block, height, current_supply);
    if (error) return error;
    
    // Validate all non-coinbase transactions
    for (size_t i = 1; i < block.transactions.size(); i++) {
        const auto& tx = block.transactions[i];
        
        // Structure
        error = TransactionValidator::ValidateStructure(tx);
        if (error) {
            return ValidationError(
                ValidationError::Type::BLOCK_INVALID_TRANSACTION,
                "Block contains invalid transaction: " + error->message
            );
        }
        
        // Against UTXO set
        error = TransactionValidator::ValidateAgainstUTXO(tx, utxo_set, height);
        if (error) {
            return ValidationError(
                ValidationError::Type::BLOCK_INVALID_TRANSACTION,
                "Block transaction validation failed: " + error->message
            );
        }
        
        // Signatures (TODO)
        error = TransactionValidator::ValidateSignatures(tx, utxo_set);
        if (error) {
            return ValidationError(
                ValidationError::Type::BLOCK_INVALID_TRANSACTION,
                "Block transaction signature invalid: " + error->message
            );
        }
    }
    
    return std::nullopt; // Valid
}

} // namespace validation
} // namespace parthenon
