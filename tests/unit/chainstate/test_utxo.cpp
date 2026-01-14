// ParthenonChain - UTXO Set Tests
// Test UTXO management operations

#include "chainstate/utxo.h"
#include "primitives/transaction.h"

#include <cassert>
#include <iostream>

using namespace parthenon::chainstate;
using namespace parthenon::primitives;

void TestCoinCreation() {
    std::cout << "Test: Coin creation and spendability" << std::endl;

    // Create a regular coin
    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 1000, pubkey);
    Coin coin(output, 100, false);

    assert(coin.height == 100);
    assert(!coin.is_coinbase);
    assert(coin.IsSpendable(100));  // Regular coin is immediately spendable
    assert(coin.IsSpendable(101));

    // Create a coinbase coin
    Coin coinbase_coin(output, 100, true);
    assert(coinbase_coin.is_coinbase);
    assert(!coinbase_coin.IsSpendable(100));  // Not yet mature
    assert(!coinbase_coin.IsSpendable(150));  // Still not mature
    assert(!coinbase_coin.IsSpendable(199));  // Still not mature
    assert(coinbase_coin.IsSpendable(200));   // Mature at height 200 (100 + 100)
    assert(coinbase_coin.IsSpendable(201));   // Still mature

    std::cout << "  ✓ Passed (coinbase maturity works)" << std::endl;
}

void TestUTXOSetBasics() {
    std::cout << "Test: UTXO set basic operations" << std::endl;

    UTXOSet utxo_set;

    // Initially empty
    assert(utxo_set.GetSize() == 0);

    // Create an outpoint and coin
    std::array<uint8_t, 32> txid{};
    txid[0] = 1;
    OutPoint outpoint(txid, 0);

    std::vector<uint8_t> pubkey(32, 0xAB);
    TxOutput output(AssetID::TALANTON, 1000, pubkey);
    Coin coin(output, 100, false);

    // Add coin
    utxo_set.AddCoin(outpoint, coin);
    assert(utxo_set.GetSize() == 1);
    assert(utxo_set.HaveCoin(outpoint));

    // Retrieve coin
    auto retrieved = utxo_set.GetCoin(outpoint);
    assert(retrieved.has_value());
    assert(retrieved->height == 100);
    assert(retrieved->output.value.amount == 1000);

    // Spend coin
    assert(utxo_set.SpendCoin(outpoint));
    assert(utxo_set.GetSize() == 0);
    assert(!utxo_set.HaveCoin(outpoint));

    // Try to spend again (should fail)
    assert(!utxo_set.SpendCoin(outpoint));

    std::cout << "  ✓ Passed (add, retrieve, spend)" << std::endl;
}

void TestMultipleCoins() {
    std::cout << "Test: Multiple coins in UTXO set" << std::endl;

    UTXOSet utxo_set;

    std::vector<uint8_t> pubkey(32, 0xAB);

    // Add 10 different coins
    for (int i = 0; i < 10; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        TxOutput output(AssetID::TALANTON, 1000 + i, pubkey);
        Coin coin(output, 100, false);

        utxo_set.AddCoin(outpoint, coin);
    }

    assert(utxo_set.GetSize() == 10);

    // Verify all coins exist
    for (int i = 0; i < 10; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        assert(utxo_set.HaveCoin(outpoint));
        auto coin = utxo_set.GetCoin(outpoint);
        assert(coin.has_value());
        assert(coin->output.value.amount == static_cast<uint64_t>(1000 + i));
    }

    // Spend some coins
    for (int i = 0; i < 5; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        assert(utxo_set.SpendCoin(outpoint));
    }

    assert(utxo_set.GetSize() == 5);

    // Verify remaining coins
    for (int i = 5; i < 10; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        assert(utxo_set.HaveCoin(outpoint));
    }

    std::cout << "  ✓ Passed (multiple coins)" << std::endl;
}

void TestClear() {
    std::cout << "Test: Clear UTXO set" << std::endl;

    UTXOSet utxo_set;

    // Add coins
    std::vector<uint8_t> pubkey(32, 0xAB);
    for (int i = 0; i < 5; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        TxOutput output(AssetID::TALANTON, 1000, pubkey);
        Coin coin(output, 100, false);
        utxo_set.AddCoin(outpoint, coin);
    }

    assert(utxo_set.GetSize() == 5);

    // Clear
    utxo_set.Clear();
    assert(utxo_set.GetSize() == 0);

    // Verify all coins are gone
    for (int i = 0; i < 5; i++) {
        std::array<uint8_t, 32> txid{};
        txid[0] = static_cast<uint8_t>(i);
        OutPoint outpoint(txid, 0);

        assert(!utxo_set.HaveCoin(outpoint));
    }

    std::cout << "  ✓ Passed (clear works)" << std::endl;
}

int main() {
    std::cout << "=== UTXO Tests ===" << std::endl;

    TestCoinCreation();
    TestUTXOSetBasics();
    TestMultipleCoins();
    TestClear();

    std::cout << "\n✓ All UTXO tests passed!" << std::endl;
    return 0;
}
