// ParthenonChain - Transaction Tests
// Test transaction structure, serialization, and validation

#include "primitives/transaction.h"

#include <cassert>
#include <iostream>

using namespace parthenon::primitives;

void TestOutPoint() {
    std::cout << "Test: OutPoint structure" << std::endl;

    std::array<uint8_t, 32> txid{};
    txid[0] = 0x01;
    txid[31] = 0xFF;

    OutPoint op(txid, 5);
    assert(op.txid == txid);
    assert(op.vout == 5);

    // Serialization
    std::vector<uint8_t> serialized;
    op.Serialize(serialized);
    assert(serialized.size() == 36); // 32 + 4

    assert(op == OutPoint::Deserialize(serialized.data()));

    std::cout << "  ✓ Passed" << std::endl;
}

void TestTxOutput() {
    std::cout << "Test: Transaction output" << std::endl;

    // Create output
    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 1000000, pubkey);

    assert(output.value.asset == AssetID::TALANTON);
    assert(output.value.amount == 1000000);
    assert(output.IsValid());

    // Serialize
    std::vector<uint8_t> serialized;
    output.Serialize(serialized);

    // Deserialize
    const uint8_t *ptr = serialized.data();
    auto output2 = TxOutput::Deserialize(ptr, serialized.data() + serialized.size());
    assert(output2.has_value());
    assert(output == *output2);

    std::cout << "  ✓ Passed" << std::endl;
}

void TestTransaction() {
    std::cout << "Test: Basic transaction" << std::endl;

    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Add input
    TxInput input;
    input.prevout.vout = 0;
    input.sequence = 0xFFFFFFFF;
    tx.inputs.push_back(input);

    // Add output
    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 500000, pubkey);
    tx.outputs.push_back(output);

    // Transaction has structural validity
    assert(tx.IsValid()); // Has inputs and outputs, no duplicates

    std::cout << "  ✓ Passed" << std::endl;
}

void TestCoinbase() {
    std::cout << "Test: Coinbase transaction" << std::endl;

    Transaction coinbase;
    coinbase.version = 1;

    // Coinbase input
    TxInput input;
    input.prevout.txid = std::array<uint8_t, 32>{}; // All zeros
    input.prevout.vout = 0xFFFFFFFF;
    input.signature_script = {0x01, 0x02, 0x03}; // Arbitrary data
    coinbase.inputs.push_back(input);

    // Coinbase outputs (mining rewards for all three assets)
    std::vector<uint8_t> pubkey(32, 0xAB);
    coinbase.outputs.push_back(TxOutput(AssetID::TALANTON, 5000000000, pubkey)); // 50 TALN
    coinbase.outputs.push_back(TxOutput(AssetID::DRACHMA, 5000000000, pubkey));  // 50 DRM
    coinbase.outputs.push_back(TxOutput(AssetID::OBOLOS, 5000000000, pubkey));   // 50 OBL

    assert(coinbase.IsCoinbase());
    assert(coinbase.IsValid());

    std::cout << "  ✓ Passed" << std::endl;
}

void TestTransactionSerialization() {
    std::cout << "Test: Transaction serialization" << std::endl;

    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Add coinbase input
    TxInput input;
    input.prevout.txid = std::array<uint8_t, 32>{};
    input.prevout.vout = 0xFFFFFFFF;
    input.signature_script = {0x04, 0xFF, 0xFF, 0x00, 0x1D, 0x01, 0x04};
    tx.inputs.push_back(input);

    // Add outputs
    std::vector<uint8_t> pubkey(32, 0xCD);
    tx.outputs.push_back(TxOutput(AssetID::TALANTON, 5000000000, pubkey));

    // Serialize
    auto serialized = tx.Serialize();
    assert(!serialized.empty());

    // Deserialize
    auto tx2 = Transaction::Deserialize(serialized.data(), serialized.size());
    assert(tx2.has_value());
    assert(tx2->version == tx.version);
    assert(tx2->inputs.size() == tx.inputs.size());
    assert(tx2->outputs.size() == tx.outputs.size());

    // Verify TXID is deterministic
    assert(tx.GetTxID() == tx2->GetTxID());

    std::cout << "  ✓ Passed (deterministic)" << std::endl;
}

void TestCompactSize() {
    std::cout << "Test: Compact size encoding" << std::endl;

    std::vector<uint8_t> buffer;

    // Small number
    WriteCompactSize(buffer, 100);
    const uint8_t *ptr1 = buffer.data();
    assert(ReadCompactSize(ptr1) == 100);

    // Medium number
    buffer.clear();
    WriteCompactSize(buffer, 1000);
    const uint8_t *ptr2 = buffer.data();
    assert(ReadCompactSize(ptr2) == 1000);

    // Large number
    buffer.clear();
    WriteCompactSize(buffer, 100000);
    const uint8_t *ptr3 = buffer.data();
    assert(ReadCompactSize(ptr3) == 100000);

    std::cout << "  ✓ Passed" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "ParthenonChain Transaction Test Suite" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    try {
        TestOutPoint();
        TestTxOutput();
        TestTransaction();
        TestCoinbase();
        TestTransactionSerialization();
        TestCompactSize();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "All Transaction tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
