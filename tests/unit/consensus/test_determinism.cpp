// ParthenonChain - Determinism Tests
// Verify that identical inputs produce identical outputs across all consensus-critical operations

#include "crypto/sha256.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace parthenon::crypto;
using namespace parthenon::primitives;

void TestSHA256Determinism() {
    std::cout << "Test: SHA-256 determinism" << std::endl;

    const char *data = "ParthenonChain deterministic test";
    size_t len = strlen(data);

    // Hash the same data multiple times
    auto hash1 = SHA256::Hash256(reinterpret_cast<const uint8_t *>(data), len);
    auto hash2 = SHA256::Hash256(reinterpret_cast<const uint8_t *>(data), len);
    auto hash3 = SHA256::Hash256(reinterpret_cast<const uint8_t *>(data), len);

    // All hashes must be identical
    assert(hash1 == hash2);
    assert(hash2 == hash3);

    std::cout << "  ✓ Passed (SHA-256 is deterministic)" << std::endl;
}

void TestSHA256dDeterminism() {
    std::cout << "Test: SHA-256d determinism" << std::endl;

    const char *data = "Block header data";
    size_t len = strlen(data);

    // Hash the same data multiple times with double-SHA256
    auto hash1 = SHA256d::Hash256d(reinterpret_cast<const uint8_t *>(data), len);
    auto hash2 = SHA256d::Hash256d(reinterpret_cast<const uint8_t *>(data), len);
    auto hash3 = SHA256d::Hash256d(reinterpret_cast<const uint8_t *>(data), len);

    // All hashes must be identical
    assert(hash1 == hash2);
    assert(hash2 == hash3);

    std::cout << "  ✓ Passed (SHA-256d is deterministic)" << std::endl;
}

void TestTaggedHashDeterminism() {
    std::cout << "Test: Tagged SHA-256 determinism" << std::endl;

    std::string tag = "ParthenonChain/Test";
    const char *data = "test data for tagged hash";
    size_t len = strlen(data);

    // Hash with the same tag and data multiple times
    auto hash1 = TaggedSHA256::HashTagged(tag, reinterpret_cast<const uint8_t *>(data), len);
    auto hash2 = TaggedSHA256::HashTagged(tag, reinterpret_cast<const uint8_t *>(data), len);
    auto hash3 = TaggedSHA256::HashTagged(tag, reinterpret_cast<const uint8_t *>(data), len);

    // All hashes must be identical
    assert(hash1 == hash2);
    assert(hash2 == hash3);

    std::cout << "  ✓ Passed (Tagged SHA-256 is deterministic)" << std::endl;
}

void TestTransactionSerializationDeterminism() {
    std::cout << "Test: Transaction serialization determinism" << std::endl;

    // Create a transaction with known inputs
    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Add a deterministic input
    TxInput input;
    input.prevout.txid.fill(0x42);
    input.prevout.vout = 0;
    input.signature_script = {0x01, 0x02, 0x03};
    input.sequence = 0xFFFFFFFF;
    tx.inputs.push_back(input);

    // Add a deterministic output
    TxOutput output;
    output.value = AssetAmount(AssetID::TALANTON, 1000000);
    output.pubkey_script = {0x04, 0x05, 0x06};
    tx.outputs.push_back(output);

    // Serialize multiple times
    auto serialized1 = tx.Serialize();
    auto serialized2 = tx.Serialize();
    auto serialized3 = tx.Serialize();

    // All serializations must be identical
    assert(serialized1 == serialized2);
    assert(serialized2 == serialized3);

    // Hash multiple times (should also be deterministic)
    auto txid1 = tx.GetTxID();
    auto txid2 = tx.GetTxID();

    assert(txid1 == txid2);

    std::cout << "  ✓ Passed (Transaction serialization is deterministic)" << std::endl;
}

void TestBlockSerializationDeterminism() {
    std::cout << "Test: Block serialization determinism" << std::endl;

    // Create a block with deterministic data
    Block block;
    block.header.version = 1;
    block.header.prev_block_hash.fill(0x00);
    block.header.merkle_root.fill(0x11);
    block.header.timestamp = 1234567890;
    block.header.bits = 0x1d00ffff;
    block.header.nonce = 42;

    // Serialize multiple times
    auto serialized1 = block.header.Serialize();
    auto serialized2 = block.header.Serialize();
    auto serialized3 = block.header.Serialize();

    // All serializations must be identical
    assert(serialized1 == serialized2);
    assert(serialized2 == serialized3);

    // Hash multiple times (should also be deterministic)
    auto hash1 = block.GetHash();
    auto hash2 = block.GetHash();

    assert(hash1 == hash2);

    std::cout << "  ✓ Passed (Block serialization is deterministic)" << std::endl;
}

void TestAmountArithmeticDeterminism() {
    std::cout << "Test: Amount arithmetic determinism" << std::endl;

    Amount a(1000000);
    Amount b(500000);

    // Perform operations multiple times
    auto sum1 = a.Add(b);
    auto sum2 = a.Add(b);
    auto sum3 = a.Add(b);

    assert(sum1.has_value() && sum2.has_value() && sum3.has_value());
    assert(sum1->GetValue() == sum2->GetValue());
    assert(sum2->GetValue() == sum3->GetValue());

    auto diff1 = a.Subtract(b);
    auto diff2 = a.Subtract(b);

    assert(diff1.has_value() && diff2.has_value());
    assert(diff1->GetValue() == diff2->GetValue());

    std::cout << "  ✓ Passed (Amount arithmetic is deterministic)" << std::endl;
}

void TestMerkleRootDeterminism() {
    std::cout << "Test: Merkle root determinism" << std::endl;

    // Create a set of transaction hashes
    std::vector<std::array<uint8_t, 32>> txHashes;
    for (int i = 0; i < 5; i++) {
        std::array<uint8_t, 32> hash;
        hash.fill(static_cast<uint8_t>(i));
        txHashes.push_back(hash);
    }

    // Compute merkle root multiple times
    auto root1 = MerkleTree::CalculateRoot(txHashes);
    auto root2 = MerkleTree::CalculateRoot(txHashes);
    auto root3 = MerkleTree::CalculateRoot(txHashes);

    // All roots must be identical
    assert(root1 == root2);
    assert(root2 == root3);

    std::cout << "  ✓ Passed (Merkle root is deterministic)" << std::endl;
}

void TestDeterministicOrdering() {
    std::cout << "Test: Deterministic data structure ordering" << std::endl;

    // Create transactions and ensure consistent ordering
    std::vector<Transaction> txs;
    for (int i = 0; i < 10; i++) {
        Transaction tx;
        tx.version = 1;
        tx.locktime = i;

        // Add dummy input/output to make it valid
        TxInput input;
        input.prevout.txid.fill(static_cast<uint8_t>(i));
        input.prevout.vout = 0;
        tx.inputs.push_back(input);

        TxOutput output;
        output.value = AssetAmount(AssetID::TALANTON, 1000);
        output.pubkey_script = {0x01};
        tx.outputs.push_back(output);

        txs.push_back(tx);
    }

    // Sort by hash multiple times
    auto sorted1 = txs;
    auto sorted2 = txs;

    std::sort(sorted1.begin(), sorted1.end(),
              [](const Transaction &a, const Transaction &b) { return a.GetTxID() < b.GetTxID(); });

    std::sort(sorted2.begin(), sorted2.end(),
              [](const Transaction &a, const Transaction &b) { return a.GetTxID() < b.GetTxID(); });

    // Verify same ordering
    for (size_t i = 0; i < sorted1.size(); i++) {
        assert(sorted1[i].GetTxID() == sorted2[i].GetTxID());
    }

    std::cout << "  ✓ Passed (Ordering is deterministic)" << std::endl;
}

void TestNoSystemDependencies() {
    std::cout << "Test: No system randomness or time dependencies in core operations" << std::endl;

    // Verify that cryptographic operations don't use system time
    // This is tested implicitly by the determinism tests above

    // Verify block timestamp must be explicitly provided (not system time)
    Block block;
    block.header.timestamp = 1234567890; // Explicit timestamp

    auto hash1 = block.GetHash();

    // Wait briefly (simulate time passing)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Hash should be the same (no system time used)
    auto hash2 = block.GetHash();
    assert(hash1 == hash2);

    std::cout << "  ✓ Passed (No system dependencies)" << std::endl;
}

int main() {
    std::cout << "=== Determinism Tests ===" << std::endl;
    std::cout << "Verifying that identical inputs produce identical outputs" << std::endl;
    std::cout << std::endl;

    TestSHA256Determinism();
    TestSHA256dDeterminism();
    TestTaggedHashDeterminism();
    TestTransactionSerializationDeterminism();
    TestBlockSerializationDeterminism();
    TestAmountArithmeticDeterminism();
    TestMerkleRootDeterminism();
    TestDeterministicOrdering();
    TestNoSystemDependencies();

    std::cout << "\n✓ All determinism tests passed!" << std::endl;
    std::cout << "Consensus operations are deterministic and reproducible." << std::endl;

    return 0;
}
