// ParthenonChain - Automated Integration Tests
// End-to-end testing for complete system functionality

#include "consensus/issuance.h"
#include "p2p/peer_database.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

#include "core/mining/miner.h"
#include "core/node/node.h"
#include "wallet/wallet.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace parthenon;

// Test utilities
namespace {
int test_count = 0;
int test_passed = 0;
int test_failed = 0;

void TEST_START(const char *name) {
    std::cout << "\n=== TEST: " << name << " ===\n";
    test_count++;
}

void TEST_PASS(const char *name) {
    std::cout << "✅ PASS: " << name << "\n";
    test_passed++;
}

void TEST_FAIL(const char *name, const char *reason) {
    std::cerr << "❌ FAIL: " << name << " - " << reason << "\n";
    test_failed++;
}

#define ASSERT_TRUE(cond, msg)                                                                     \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            TEST_FAIL(__func__, msg);                                                              \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

#define ASSERT_EQ(a, b, msg)                                                                       \
    do {                                                                                           \
        if ((a) != (b)) {                                                                          \
            TEST_FAIL(__func__, msg);                                                              \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

// Easy difficulty for testing - allows quick nonce finding
constexpr uint32_t EASY_TEST_DIFFICULTY_BITS = 0x207fffff;

// Helper to generate a deterministic seed for testing purposes only
// NOT cryptographically secure - use only in tests
std::array<uint8_t, 32> GenerateTestSeedDeterministic(uint8_t seed_byte = 0x42) {
    std::array<uint8_t, 32> seed;
    for (size_t i = 0; i < 32; i++) {
        seed[i] = static_cast<uint8_t>((seed_byte + i) & 0xFF);
    }
    return seed;
}
} // namespace

// Test 1: Complete block production and validation flow
bool test_block_production_flow() {
    TEST_START("Block Production Flow");

    // Create temporary test directory
    std::filesystem::create_directories("/tmp/test_block_prod");

    // Initialize chainstate
    chainstate::ChainState chain_state;

    // Create wallet for coinbase
    auto seed = GenerateTestSeedDeterministic(0x11);
    wallet::Wallet wallet(seed);
    auto address = wallet.GenerateAddress("mining");

    // Create miner
    mining::Miner miner(chain_state, address.pubkey);

    // Create block template
    auto template_opt = miner.CreateBlockTemplate();
    ASSERT_TRUE(template_opt.has_value(), "Failed to create block template");

    // Verify template structure
    auto &block_template = *template_opt;
    ASSERT_TRUE(block_template.block.transactions.size() > 0, "Template should have transactions");
    ASSERT_TRUE(block_template.block.transactions[0].IsCoinbase(), "First tx should be coinbase");
    ASSERT_EQ(block_template.height, 1u, "First block should be height 1");

    // For testing, manually create a valid block without full PoW mining
    // Set difficulty to minimum and use nonce 0
    auto block = block_template.block;
    block.header.bits = EASY_TEST_DIFFICULTY_BITS;
    block.header.nonce = 0;

    // Try a few nonces to find one that works (much faster than full mining)
    bool found = false;
    for (uint32_t nonce = 0; nonce < 1000000; nonce++) {
        block.header.nonce = nonce;
        if (block.header.MeetsDifficultyTarget()) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found, "Failed to find valid nonce");

    // Verify block structure
    ASSERT_TRUE(block.IsValid(), "Block should be valid");

    // Validate and apply block to chainstate
    ASSERT_TRUE(chain_state.ValidateBlock(block), "Block validation failed");
    ASSERT_TRUE(chain_state.ApplyBlock(block), "Failed to apply block");

    // Verify chainstate updated
    ASSERT_EQ(chain_state.GetHeight(), 1u, "Chain height should be 1");

    // Verify block hash meets difficulty
    ASSERT_TRUE(block.header.MeetsDifficultyTarget(), "Block doesn't meet difficulty");

    // Cleanup
    std::filesystem::remove_all("/tmp/test_block_prod");

    TEST_PASS("Block Production Flow");
    return true;
}

#if 0 // Disabled: legacy API coverage scaffold
// Test 2: Complete transaction flow from creation to confirmation
bool test_transaction_flow() {
    TEST_START("Transaction Flow");
    
    std::filesystem::create_directories("/tmp/test_tx_flow");
    
    // Setup node and wallet
    node::Node node("/tmp/test_tx_flow", 18334);
    ASSERT_TRUE(node.Start(), "Node failed to start");
    
    crypto::Seed seed1 = crypto::GenerateSeed();
    crypto::Seed seed2 = crypto::GenerateSeed();
    wallet::Wallet wallet1(seed1);
    wallet::Wallet wallet2(seed2);
    
    auto addr1 = wallet1.GetNewAddress();
    auto addr2 = wallet2.GetNewAddress();
    
    // Mine block to give wallet1 some coins
    mining::Miner miner;
    auto block1 = miner.MineBlock(addr1, 0, {});
    ASSERT_TRUE(node.ProcessBlock(*block1, "local"), "Failed to process genesis block");
    
    // Sync wallet
    wallet1.ProcessBlock(*block1, 0);
    uint64_t balance = wallet1.GetBalance(AssetID::TALANTON);
    ASSERT_TRUE(balance > 0, "Wallet should have balance after mining");
    
    // Create transaction
    uint64_t send_amount = balance / 2;
    auto tx = wallet1.CreateTransaction(addr2, AssetID::TALANTON, send_amount);
    ASSERT_TRUE(tx != nullptr, "Failed to create transaction");
    
    // Submit transaction to node
    ASSERT_TRUE(node.SubmitTransaction(*tx), "Failed to submit transaction");
    
    // Mine block with transaction
    std::vector<primitives::Transaction> txs = {*tx};
    auto block2 = miner.MineBlock(addr1, 1, txs);
    ASSERT_TRUE(node.ProcessBlock(*block2, "local"), "Failed to process block with tx");
    
    // Sync wallets
    wallet1.ProcessBlock(*block2, 1);
    wallet2.ProcessBlock(*block2, 1);
    
    // Verify balances
    uint64_t balance2 = wallet2.GetBalance(AssetID::TALANTON);
    ASSERT_TRUE(balance2 >= send_amount, "Recipient should have received coins");
    
    node.Stop();
    std::filesystem::remove_all("/tmp/test_tx_flow");
    
    TEST_PASS("Transaction Flow");
    return true;
}
#endif

#if 0 // Disabled: legacy API coverage scaffold
// Test 3: Multi-node network synchronization
bool test_network_sync() {
    TEST_START("Network Synchronization");
    
    std::filesystem::create_directories("/tmp/test_net_sync1");
    std::filesystem::create_directories("/tmp/test_net_sync2");
    
    // Start first node
    node::Node node1("/tmp/test_net_sync1", 18335);
    ASSERT_TRUE(node1.Start(), "Node1 failed to start");
    
    // Mine some blocks on node1
    crypto::Seed seed = crypto::GenerateSeed();
    wallet::Wallet wallet(seed);
    auto addr = wallet.GetNewAddress();
    
    mining::Miner miner;
    for (uint32_t i = 0; i < 5; i++) {
        auto block = miner.MineBlock(addr, i, {});
        ASSERT_TRUE(node1.ProcessBlock(*block, "local"), "Failed to process block on node1");
    }
    
    ASSERT_EQ(node1.GetHeight(), 4u, "Node1 should have 5 blocks (0-4)");
    
    // Start second node
    node::Node node2("/tmp/test_net_sync2", 18336);
    ASSERT_TRUE(node2.Start(), "Node2 failed to start");
    
    // Connect nodes
    node2.AddPeer("127.0.0.1", 18335);
    
    // Wait for sync (in real implementation, this would be automatic)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Note: Full sync would require P2P messages to be processed
    // This test verifies the framework is in place
    
    node1.Stop();
    node2.Stop();
    
    std::filesystem::remove_all("/tmp/test_net_sync1");
    std::filesystem::remove_all("/tmp/test_net_sync2");
    
    TEST_PASS("Network Synchronization");
    return true;
}
#endif

#if 0 // Disabled: legacy API coverage scaffold
// Test 4: Smart contract deployment and execution
bool test_smart_contract_flow() {
    TEST_START("Smart Contract Flow");
    
    std::filesystem::create_directories("/tmp/test_contract");
    
    node::Node node("/tmp/test_contract", 18337);
    ASSERT_TRUE(node.Start(), "Node failed to start");
    
    // Create wallet
    crypto::Seed seed = crypto::GenerateSeed();
    wallet::Wallet wallet(seed);
    auto addr = wallet.GetNewAddress();
    
    // Mine initial block for funding
    mining::Miner miner;
    auto block1 = miner.MineBlock(addr, 0, {});
    ASSERT_TRUE(node.ProcessBlock(*block1, "local"), "Failed to process genesis");
    
    wallet.ProcessBlock(*block1, 0);
    
    // Simple contract bytecode (example: counter contract)
    std::vector<uint8_t> contract_code = {
        0x60, 0x00, // PUSH1 0x00 (counter value)
        0x60, 0x01, // PUSH1 0x01 (increment)
        0x01,       // ADD
        0x60, 0x00, // PUSH1 0x00 (storage slot)
        0x55,       // SSTORE
    };
    
    // Contract deployment costs
    constexpr uint64_t CONTRACT_GAS_COST = 1000000;     // Gas for deployment (OBOLOS)
    constexpr uint64_t MINING_REWARD = 100000000;       // Block reward (TALANTON)
    
    // Create contract deployment transaction
    // In a UTXO model, contract deployment is done via a special output
    primitives::Transaction deploy_tx;
    deploy_tx.version = 1;
    deploy_tx.locktime = 0;
    
    // Input: Spend from wallet (simplified - would use actual UTXO)
    primitives::TxInput input;
    input.prevout.txid = block1->transactions[0].GetTxID();
    input.prevout.vout = 0;
    input.sequence = 0xFFFFFFFF;
    // Signature would be added here in real implementation
    deploy_tx.inputs.push_back(input);
    
    // Output 1: Contract deployment (OBOLOS asset with contract code in pubkey_script)
    primitives::TxOutput contract_output;
    contract_output.value = primitives::AssetAmount(
        primitives::AssetID::OBOLOS,
        CONTRACT_GAS_COST
    );
    // Contract deployment uses bytecode as the pubkey_script
    contract_output.pubkey_script = contract_code;
    deploy_tx.outputs.push_back(contract_output);
    
    // Output 2: Change output (return remaining funds)
    // In real implementation, would calculate: input_amount - gas_cost - fee
    primitives::TxOutput change_output;
    change_output.value = primitives::AssetAmount(
        primitives::AssetID::TALANTON,
        MINING_REWARD - CONTRACT_GAS_COST  // Simplified: return most of mining reward
    );
    change_output.pubkey_script = addr.pubkey;  // Return to wallet
    deploy_tx.outputs.push_back(change_output);
    
    // Note: Contract execution would be tested here
    // Framework is in place in EVM module
    
    node.Stop();
    std::filesystem::remove_all("/tmp/test_contract");
    
    TEST_PASS("Smart Contract Flow");
    return true;
}
#endif

// Test 5: Peer database and scoring system
bool test_peer_database() {
    TEST_START("Peer Database and Scoring");

    std::filesystem::create_directories("/tmp/test_peer_db");

    p2p::PeerDatabase db;
    ASSERT_TRUE(db.Open("/tmp/test_peer_db/peers.dat"), "Failed to open peer database");

    // Add peers
    db.AddPeer("192.168.1.100", 8333, 1);
    db.AddPeer("192.168.1.101", 8333, 1);
    db.AddPeer("192.168.1.102", 8333, 1);

    ASSERT_EQ(db.GetPeerCount(), 3u, "Should have 3 peers");

    // Test connection tracking
    db.RecordConnectionAttempt("192.168.1.100", 8333);
    db.RecordSuccessfulConnection("192.168.1.100", 8333);

    db.RecordConnectionAttempt("192.168.1.101", 8333);
    db.RecordFailedConnection("192.168.1.101", 8333);

    // Test scoring
    db.RecordBlockReceived("192.168.1.100", 8333);
    db.RecordTxReceived("192.168.1.100", 8333);
    db.RecordInvalidMessage("192.168.1.101", 8333);

    // Get good peers (should prefer peer with higher score)
    auto good_peers = db.GetGoodPeers(10);
    ASSERT_TRUE(good_peers.size() > 0, "Should have good peers");
    ASSERT_TRUE(good_peers[0].score > 50.0, "Top peer should have high score");

    // Test banning
    db.BanPeer("192.168.1.102", 8333, 3600);
    ASSERT_TRUE(db.IsBanned("192.168.1.102", 8333), "Peer should be banned");
    ASSERT_EQ(db.GetBannedCount(), 1u, "Should have 1 banned peer");

    // Close and reopen to test persistence
    db.Close();

    p2p::PeerDatabase db2;
    ASSERT_TRUE(db2.Open("/tmp/test_peer_db/peers.dat"), "Failed to reopen database");
    ASSERT_EQ(db2.GetPeerCount(), 3u, "Peers should persist");
    ASSERT_TRUE(db2.IsBanned("192.168.1.102", 8333), "Ban should persist");

    db2.Close();
    std::filesystem::remove_all("/tmp/test_peer_db");

    TEST_PASS("Peer Database and Scoring");
    return true;
}

#if 0 // Disabled: legacy API coverage scaffold
// Test 6: UTXO persistence and wallet synchronization
bool test_utxo_persistence() {
    TEST_START("UTXO Persistence");
    
    std::filesystem::create_directories("/tmp/test_utxo");
    
    node::Node node("/tmp/test_utxo", 18338);
    ASSERT_TRUE(node.Start(), "Node failed to start");
    
    // Create wallets
    crypto::Seed seed1 = crypto::GenerateSeed();
    wallet::Wallet wallet1(seed1);
    auto addr1 = wallet1.GetNewAddress();
    
    // Mine blocks
    mining::Miner miner;
    for (uint32_t i = 0; i < 3; i++) {
        auto block = miner.MineBlock(addr1, i, {});
        ASSERT_TRUE(node.ProcessBlock(*block, "local"), "Failed to process block");
        wallet1.ProcessBlock(*block, i);
    }
    
    uint64_t balance_before = wallet1.GetBalance(AssetID::TALANTON);
    ASSERT_TRUE(balance_before > 0, "Wallet should have balance");
    
    // Stop and restart node
    node.Stop();
    
    node::Node node2("/tmp/test_utxo", 18338);
    ASSERT_TRUE(node2.Start(), "Failed to restart node");
    ASSERT_EQ(node2.GetHeight(), 2u, "Chain should persist");
    
    // Sync new wallet instance
    crypto::Seed seed_copy = seed1; // Same seed
    wallet::Wallet wallet2(seed_copy);
    // wallet2.SyncWithChain() would be called here
    
    node2.Stop();
    std::filesystem::remove_all("/tmp/test_utxo");
    
    TEST_PASS("UTXO Persistence");
    return true;
}
#endif

// Test 7: Performance - Block validation throughput
// Disabled: performance test uses legacy types/methods not in current APIs
/*
bool test_performance_validation() {
    TEST_START("Performance - Block Validation");

    std::filesystem::create_directories("/tmp/test_perf");

    node::Node node("/tmp/test_perf", 18339);
    ASSERT_TRUE(node.Start(), "Node failed to start");

    crypto::Seed seed = crypto::GenerateSeed();
    wallet::Wallet wallet(seed);
    auto addr = wallet.GetNewAddress();

    mining::Miner miner;

    // Measure block processing time
    auto start = std::chrono::high_resolution_clock::now();

    const uint32_t num_blocks = 100;
    for (uint32_t i = 0; i < num_blocks; i++) {
        auto block = miner.MineBlock(addr, i, {});
        ASSERT_TRUE(node.ProcessBlock(*block, "local"), "Failed to process block");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double blocks_per_second = (num_blocks * 1000.0) / duration.count();
    std::cout << "Block validation throughput: " << blocks_per_second << " blocks/second\n";
    std::cout << "Average time per block: " << (duration.count() / num_blocks) << " ms\n";

    ASSERT_TRUE(blocks_per_second > 10.0, "Should process at least 10 blocks/second");

    node.Stop();
    std::filesystem::remove_all("/tmp/test_perf");

    TEST_PASS("Performance - Block Validation");
    return true;
}
*/

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PantheonChain - Automated Integration Test Suite       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    // Run all tests
    test_block_production_flow();
    // test_transaction_flow();  // Disabled: requires wallet transaction creation API migration
    // test_network_sync();  // Disabled: requires multi-node P2P harness
    // test_smart_contract_flow();  // Disabled: requires contract deployment harness
    test_peer_database();
    // test_utxo_persistence();  // Disabled: requires node restart persistence harness
    // test_performance_validation();  // Disabled: legacy benchmark requires API updates

    // Print summary
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Test Summary                                            ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Total Tests:  " << test_count
              << "                                           ║\n";
    std::cout << "║  Passed:       " << test_passed
              << "                                           ║\n";
    std::cout << "║  Failed:       " << test_failed
              << "                                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    return (test_failed == 0) ? 0 : 1;
}
