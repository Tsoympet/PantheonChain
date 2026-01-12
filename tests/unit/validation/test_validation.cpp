// ParthenonChain - Validation Tests
// Test transaction and block validation

#include "validation/validation.h"
#include "consensus/difficulty.h"
#include "consensus/issuance.h"
#include <iostream>
#include <cassert>

using namespace parthenon::validation;
using namespace parthenon::chainstate;
using namespace parthenon::primitives;
using namespace parthenon::consensus;

void TestTransactionStructureValidation() {
    std::cout << "Test: Transaction structure validation" << std::endl;
    
    // Valid transaction
    Transaction tx;
    tx.version = 1;
    
    std::vector<uint8_t> pubkey(32, 0xAB);
    std::array<uint8_t, 32> txid{};
    txid[0] = 1;
    
    TxInput input;
    input.prevout = OutPoint(txid, 0);
    tx.inputs.push_back(input);
    
    tx.outputs.push_back(TxOutput(AssetID::TALANTON, 1000, pubkey));
    
    auto error = TransactionValidator::ValidateStructure(tx);
    assert(!error.has_value());
    
    // Transaction with no outputs
    Transaction no_outputs_tx = tx;
    no_outputs_tx.outputs.clear();
    error = TransactionValidator::ValidateStructure(no_outputs_tx);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::TX_NO_OUTPUTS);
    
    // Transaction with duplicate inputs
    Transaction dup_inputs_tx = tx;
    dup_inputs_tx.inputs.push_back(input); // Same input twice
    error = TransactionValidator::ValidateStructure(dup_inputs_tx);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::TX_DUPLICATE_INPUTS);
    
    std::cout << "  ✓ Passed (structure validation)" << std::endl;
}

void TestTransactionUTXOValidation() {
    std::cout << "Test: Transaction UTXO validation" << std::endl;
    
    UTXOSet utxo_set;
    
    // Add a coin to UTXO set
    std::array<uint8_t, 32> txid{};
    txid[0] = 1;
    OutPoint outpoint(txid, 0);
    
    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 1000, pubkey);
    Coin coin(output, 100, false);
    utxo_set.AddCoin(outpoint, coin);
    
    // Create transaction spending this coin
    Transaction tx;
    tx.version = 1;
    
    TxInput input;
    input.prevout = outpoint;
    tx.inputs.push_back(input);
    
    // Output with less amount (valid)
    tx.outputs.push_back(TxOutput(AssetID::TALANTON, 900, pubkey));
    
    auto error = TransactionValidator::ValidateAgainstUTXO(tx, utxo_set, 150);
    assert(!error.has_value());
    
    // Output with more amount (invalid - creates assets)
    Transaction invalid_tx = tx;
    invalid_tx.outputs[0].value.amount = 1100;
    error = TransactionValidator::ValidateAgainstUTXO(invalid_tx, utxo_set, 150);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::TX_ASSET_CONSERVATION);
    
    // Spending non-existent input
    Transaction missing_input_tx;
    missing_input_tx.version = 1;
    
    std::array<uint8_t, 32> fake_txid{};
    fake_txid[0] = 2;
    TxInput fake_input;
    fake_input.prevout = OutPoint(fake_txid, 0);
    missing_input_tx.inputs.push_back(fake_input);
    missing_input_tx.outputs.push_back(TxOutput(AssetID::TALANTON, 100, pubkey));
    
    error = TransactionValidator::ValidateAgainstUTXO(missing_input_tx, utxo_set, 150);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::TX_MISSING_INPUT);
    
    std::cout << "  ✓ Passed (UTXO validation)" << std::endl;
}

void TestCoinbaseMaturity() {
    std::cout << "Test: Coinbase maturity enforcement" << std::endl;
    
    UTXOSet utxo_set;
    
    // Add a coinbase coin at height 100
    std::array<uint8_t, 32> txid{};
    txid[0] = 1;
    OutPoint outpoint(txid, 0);
    
    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 5000000000, pubkey);
    Coin coin(output, 100, true); // Coinbase coin
    utxo_set.AddCoin(outpoint, coin);
    
    // Try to spend at height 150 (not yet mature)
    Transaction tx;
    tx.version = 1;
    
    TxInput input;
    input.prevout = outpoint;
    tx.inputs.push_back(input);
    tx.outputs.push_back(TxOutput(AssetID::TALANTON, 1000, pubkey));
    
    auto error = TransactionValidator::ValidateAgainstUTXO(tx, utxo_set, 150);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::TX_IMMATURE_COINBASE);
    
    // Try to spend at height 200 (mature)
    error = TransactionValidator::ValidateAgainstUTXO(tx, utxo_set, 200);
    assert(!error.has_value());
    
    std::cout << "  ✓ Passed (coinbase maturity)" << std::endl;
}

void TestBlockStructureValidation() {
    std::cout << "Test: Block structure validation" << std::endl;
    
    // Valid block
    Block block;
    block.header.version = 1;
    block.header.bits = Difficulty::GetInitialBits();
    
    Transaction coinbase;
    coinbase.version = 1;
    
    TxInput coinbase_input;
    coinbase_input.prevout.txid = std::array<uint8_t, 32>{};
    coinbase_input.prevout.vout = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);
    
    std::vector<uint8_t> pubkey(32, 0xAB);
    coinbase.outputs.push_back(TxOutput(AssetID::TALANTON, 5000000000, pubkey));
    
    block.transactions.push_back(coinbase);
    block.header.merkle_root = block.CalculateMerkleRoot();
    
    auto error = BlockValidator::ValidateStructure(block);
    assert(!error.has_value());
    
    // Block with no transactions
    Block no_tx_block;
    error = BlockValidator::ValidateStructure(no_tx_block);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::BLOCK_NO_TRANSACTIONS);
    
    // Block with non-coinbase first transaction
    Block no_coinbase_block = block;
    no_coinbase_block.transactions[0].inputs[0].prevout.vout = 0; // Make it non-coinbase
    error = BlockValidator::ValidateStructure(no_coinbase_block);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::BLOCK_NO_COINBASE);
    
    std::cout << "  ✓ Passed (block structure)" << std::endl;
}

void TestCoinbaseRewardValidation() {
    std::cout << "Test: Coinbase reward validation" << std::endl;
    
    Block block;
    block.header.version = 1;
    
    Transaction coinbase;
    coinbase.version = 1;
    
    TxInput coinbase_input;
    coinbase_input.prevout.txid = std::array<uint8_t, 32>{};
    coinbase_input.prevout.vout = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);
    
    std::vector<uint8_t> pubkey(32, 0xAB);
    
    // Valid reward
    uint64_t valid_reward = Issuance::GetBlockReward(0, AssetID::TALANTON);
    coinbase.outputs.push_back(TxOutput(AssetID::TALANTON, valid_reward, pubkey));
    
    block.transactions.push_back(coinbase);
    block.header.merkle_root = block.CalculateMerkleRoot();
    
    std::map<AssetID, uint64_t> current_supply;
    current_supply[AssetID::TALANTON] = 0;
    current_supply[AssetID::DRACHMA] = 0;
    current_supply[AssetID::OBOLOS] = 0;
    
    auto error = BlockValidator::ValidateCoinbase(block, 0, current_supply);
    assert(!error.has_value());
    
    // Excessive reward
    Block excessive_block = block;
    excessive_block.transactions[0].outputs[0].value.amount = valid_reward + 1;
    error = BlockValidator::ValidateCoinbase(excessive_block, 0, current_supply);
    assert(error.has_value());
    assert(error->type == ValidationError::Type::BLOCK_INVALID_COINBASE_REWARD);
    
    std::cout << "  ✓ Passed (coinbase reward)" << std::endl;
}

int main() {
    std::cout << "=== Validation Tests ===" << std::endl;
    
    TestTransactionStructureValidation();
    TestTransactionUTXOValidation();
    TestCoinbaseMaturity();
    TestBlockStructureValidation();
    TestCoinbaseRewardValidation();
    
    std::cout << "\n✓ All validation tests passed!" << std::endl;
    return 0;
}
