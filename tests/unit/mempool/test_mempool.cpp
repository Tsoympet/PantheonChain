// ParthenonChain - Mempool Tests
// Test transaction pool management

#include "chainstate/utxo.h"
#include "mempool/mempool.h"
#include "primitives/transaction.h"

#include <cassert>
#include <iostream>

using namespace parthenon::mempool;
using namespace parthenon::chainstate;
using namespace parthenon::primitives;

// Helper to create a transaction
Transaction CreateTestTransaction(uint8_t id, uint64_t amount) {
    Transaction tx;
    tx.version = 1;

    // Input
    std::array<uint8_t, 32> prev_txid{};
    prev_txid[0] = id;
    TxInput input;
    input.prevout = OutPoint(prev_txid, 0);
    tx.inputs.push_back(input);

    // Output
    std::vector<uint8_t> pubkey(32, 0xAB);
    tx.outputs.push_back(TxOutput(AssetID::TALANTON, amount, pubkey));

    return tx;
}

void TestMempoolBasics() {
    std::cout << "Test: Mempool basic operations" << std::endl;

    Mempool mempool;
    UTXOSet utxo_set;

    // Create UTXO for spending
    std::array<uint8_t, 32> utxo_txid{};
    utxo_txid[0] = 1;
    OutPoint outpoint(utxo_txid, 0);

    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 10000, pubkey);
    Coin coin(output, 100, false);
    utxo_set.AddCoin(outpoint, coin);

    // Create a transaction
    auto tx = CreateTestTransaction(1, 9000);  // 1000 fee

    // Add to mempool
    assert(mempool.AddTransaction(tx, utxo_set, 150));

    // Verify it's in mempool
    auto txid = tx.GetTxID();
    assert(mempool.HasTransaction(txid));
    assert(mempool.GetTransactionCount() == 1);

    // Get transaction
    auto retrieved = mempool.GetTransaction(txid);
    assert(retrieved.has_value());

    // Remove transaction
    assert(mempool.RemoveTransaction(txid));
    assert(!mempool.HasTransaction(txid));
    assert(mempool.GetTransactionCount() == 0);

    std::cout << "  ✓ Passed (basic operations)" << std::endl;
}

void TestMempoolPriority() {
    std::cout << "Test: Mempool fee-based priority" << std::endl;

    Mempool mempool;
    UTXOSet utxo_set;

    // Create multiple UTXOs
    for (int i = 1; i <= 3; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        std::vector<uint8_t> pubkey(32, 0xAB);
        TxOutput output(AssetID::TALANTON, 10000, pubkey);
        Coin coin(output, 100, false);
        utxo_set.AddCoin(outpoint, coin);
    }

    // Add transactions with different fees
    auto tx1 = CreateTestTransaction(1, 9900);  // Low fee (100)
    auto tx2 = CreateTestTransaction(2, 9000);  // High fee (1000)
    auto tx3 = CreateTestTransaction(3, 9500);  // Medium fee (500)

    mempool.AddTransaction(tx1, utxo_set, 150);
    mempool.AddTransaction(tx2, utxo_set, 150);
    mempool.AddTransaction(tx3, utxo_set, 150);

    // Get transactions by fee rate
    auto txs = mempool.GetTransactionsByFeeRate(10);
    assert(txs.size() == 3);

    // Should be ordered by fee rate (highest first)
    // tx2 (1000 fee) should be first
    assert(txs[0].GetTxID() == tx2.GetTxID());

    std::cout << "  ✓ Passed (priority ordering)" << std::endl;
}

void TestMempoolConflictDetection() {
    std::cout << "Test: Mempool conflict detection" << std::endl;

    Mempool mempool;
    UTXOSet utxo_set;

    // Create UTXO
    std::array<uint8_t, 32> txid{};
    txid[0] = 1;
    OutPoint outpoint(txid, 0);

    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 10000, pubkey);
    Coin coin(output, 100, false);
    utxo_set.AddCoin(outpoint, coin);

    // Add first transaction
    auto tx1 = CreateTestTransaction(1, 9000);
    assert(mempool.AddTransaction(tx1, utxo_set, 150));

    // Try to add conflicting transaction (same input)
    auto tx2 = CreateTestTransaction(1, 8000);
    assert(!mempool.AddTransaction(tx2, utxo_set, 150));  // Should fail (double-spend)

    std::cout << "  ✓ Passed (conflict detection)" << std::endl;
}

void TestMempoolSizeLimit() {
    std::cout << "Test: Mempool size limit" << std::endl;

    Mempool mempool;
    mempool.SetMaxSize(500);  // Very small limit

    UTXOSet utxo_set;

    // Create many UTXOs
    for (int i = 1; i <= 10; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        std::vector<uint8_t> pubkey(32, 0xAB);
        TxOutput output(AssetID::TALANTON, 10000, pubkey);
        Coin coin(output, 100, false);
        utxo_set.AddCoin(outpoint, coin);
    }

    // Add transactions until mempool is full
    int added = 0;
    for (int i = 1; i <= 10; i++) {
        auto tx = CreateTestTransaction(static_cast<uint8_t>(i), 9000);
        if (mempool.AddTransaction(tx, utxo_set, 150)) {
            added++;
        } else {
            break;  // Mempool full
        }
    }

    // Should have hit size limit before adding all
    assert(mempool.GetSize() <= 500);
    std::cout << "    Added " << added << " transactions before hitting size limit" << std::endl;

    std::cout << "  ✓ Passed (size limit)" << std::endl;
}

void TestMempoolClear() {
    std::cout << "Test: Mempool clear" << std::endl;

    Mempool mempool;
    UTXOSet utxo_set;

    // Add some transactions
    for (int i = 1; i <= 3; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        std::vector<uint8_t> pubkey(32, 0xAB);
        TxOutput output(AssetID::TALANTON, 10000, pubkey);
        Coin coin(output, 100, false);
        utxo_set.AddCoin(outpoint, coin);

        auto tx = CreateTestTransaction(static_cast<uint8_t>(i), 9000);
        mempool.AddTransaction(tx, utxo_set, 150);
    }

    assert(mempool.GetTransactionCount() == 3);

    // Clear
    mempool.Clear();
    assert(mempool.GetTransactionCount() == 0);
    assert(mempool.GetSize() == 0);

    std::cout << "  ✓ Passed (clear)" << std::endl;
}

int main() {
    std::cout << "=== Mempool Tests ===" << std::endl;

    TestMempoolBasics();
    TestMempoolPriority();
    TestMempoolConflictDetection();
    TestMempoolSizeLimit();
    TestMempoolClear();

    std::cout << "\n✓ All mempool tests passed!" << std::endl;
    return 0;
}
