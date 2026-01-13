#include "layer2/bridges/channels/payment_channel.h"
#include "layer2/bridges/htlc/htlc.h"
#include "layer2/bridges/spv/spv_bridge.h"
#include <cassert>
#include <iostream>

using namespace parthenon::layer2;

void test_payment_channel() {
    std::cout << "Testing payment channels..." << std::endl;
    
    // Create test pubkeys
    std::vector<uint8_t> pubkey_a(32, 0xAA);
    std::vector<uint8_t> pubkey_b(32, 0xBB);
    
    // Initial balances
    ChannelBalance balance_a(1000, 500, 250);
    ChannelBalance balance_b(500, 1000, 750);
    
    // Create channel
    PaymentChannel channel(pubkey_a, pubkey_b, balance_a, balance_b);
    
    // Test initial state
    assert(channel.GetState() == ChannelState::FUNDING);
    
    // Open channel
    assert(channel.Open());
    assert(channel.GetState() == ChannelState::OPEN);
    
    // Test balance verification
    assert(channel.VerifyBalances());
    
    // Update state (transfer 100 TALN from A to B)
    ChannelBalance new_balance_a(900, 500, 250);
    ChannelBalance new_balance_b(600, 1000, 750);
    std::vector<uint8_t> sig_a(64, 0x01);
    std::vector<uint8_t> sig_b(64, 0x02);
    
    assert(channel.UpdateState(new_balance_a, new_balance_b, 1, sig_a, sig_b));
    assert(channel.GetSequence() == 1);
    assert(channel.VerifyBalances());
    
    // Initiate close
    assert(channel.InitiateClose(86400));  // 1 day dispute period
    assert(channel.GetState() == ChannelState::CLOSING);
    
    std::cout << "Payment channel tests passed!" << std::endl;
}

void test_htlc() {
    std::cout << "Testing HTLCs..." << std::endl;
    
    // Create hash lock
    std::vector<uint8_t> preimage = {1, 2, 3, 4, 5};
    auto hash_arr = parthenon::crypto::SHA256::Hash256(preimage);
    std::vector<uint8_t> hash_lock(hash_arr.begin(), hash_arr.end());
    
    std::vector<uint8_t> sender(32, 0xAA);
    std::vector<uint8_t> receiver(32, 0xBB);
    
    // Create HTLC
    HTLC htlc(hash_lock, 3600, 1000, sender, receiver);
    
    // Test preimage verification
    assert(htlc.VerifyPreimage(preimage));
    
    std::vector<uint8_t> wrong_preimage = {5, 4, 3, 2, 1};
    assert(!htlc.VerifyPreimage(wrong_preimage));
    
    // Test claim with preimage
    assert(htlc.ClaimWithPreimage(preimage));
    
    std::cout << "HTLC tests passed!" << std::endl;
}

void test_htlc_routing() {
    std::cout << "Testing HTLC routing..." << std::endl;
    
    std::vector<uint8_t> payment_hash(32, 0xFF);
    HTLCRoute route(payment_hash, 10000);
    
    // Add hops
    std::vector<uint8_t> node1(32, 0x01);
    std::vector<uint8_t> node2(32, 0x02);
    std::vector<uint8_t> node3(32, 0x03);
    
    route.AddHop(RouteHop(node1, 10, 100));
    route.AddHop(RouteHop(node2, 15, 50));
    route.AddHop(RouteHop(node3, 5, 25));
    
    // Validate route
    assert(route.Validate());
    assert(route.GetTotalFees() == 30);
    assert(route.GetHops().size() == 3);
    
    std::cout << "HTLC routing tests passed!" << std::endl;
}

void test_spv_merkle_proof() {
    std::cout << "Testing SPV Merkle proofs..." << std::endl;
    
    // Create test transaction hashes
    std::vector<std::vector<uint8_t>> tx_hashes;
    for (int i = 0; i < 4; ++i) {
        std::vector<uint8_t> hash(32, static_cast<uint8_t>(i));
        tx_hashes.push_back(hash);
    }
    
    // Compute merkle root
    std::vector<uint8_t> root = SPVBridge::ComputeMerkleRoot(tx_hashes);
    assert(root.size() == 32);
    
    // Build proof for first transaction
    MerkleProof proof = SPVBridge::BuildMerkleProof(tx_hashes[0], tx_hashes);
    
    // Verify proof
    assert(SPVBridge::VerifyMerkleProof(proof, root));
    
    // Test with wrong root
    std::vector<uint8_t> wrong_root(32, 0xFF);
    assert(!SPVBridge::VerifyMerkleProof(proof, wrong_root));
    
    std::cout << "SPV Merkle proof tests passed!" << std::endl;
}

int main() {
    try {
        test_payment_channel();
        test_htlc();
        test_htlc_routing();
        test_spv_merkle_proof();
        
        std::cout << "\n✓ All Layer 2 tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
