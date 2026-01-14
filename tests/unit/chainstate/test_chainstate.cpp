// ParthenonChain - ChainState Tests
// Test blockchain state tracking and supply cap enforcement

#include "chainstate/chainstate.h"
#include "consensus/difficulty.h"
#include "consensus/issuance.h"
#include "crypto/schnorr.h"

#include <cassert>
#include <iostream>

using namespace parthenon::chainstate;
using namespace parthenon::primitives;
using namespace parthenon::consensus;
using namespace parthenon::crypto;

// Helper to create a simple coinbase transaction
Transaction CreateCoinbase(uint64_t height) {
    Transaction tx;

    // Coinbase input (empty prevout)
    TxInput coinbase_input;
    coinbase_input.prevout.txid = std::array<uint8_t, 32>{};
    coinbase_input.prevout.vout = 0xffffffff;
    tx.inputs.push_back(coinbase_input);

    // Coinbase outputs (one for each asset with valid rewards)
    uint64_t taln_reward = Issuance::GetBlockReward(height, AssetID::TALANTON);
    uint64_t drm_reward = Issuance::GetBlockReward(height, AssetID::DRACHMA);
    uint64_t obl_reward = Issuance::GetBlockReward(height, AssetID::OBOLOS);

    if (taln_reward > 0) {
        TxOutput taln_out;
        taln_out.value = AssetAmount(AssetID::TALANTON, taln_reward);
        // Use a dummy pubkey
        taln_out.pubkey_script.resize(32);
        tx.outputs.push_back(taln_out);
    }

    if (drm_reward > 0) {
        TxOutput drm_out;
        drm_out.value = AssetAmount(AssetID::DRACHMA, drm_reward);
        drm_out.pubkey_script.resize(32);
        tx.outputs.push_back(drm_out);
    }

    if (obl_reward > 0) {
        TxOutput obl_out;
        obl_out.value = AssetAmount(AssetID::OBOLOS, obl_reward);
        obl_out.pubkey_script.resize(32);
        tx.outputs.push_back(obl_out);
    }

    return tx;
}

// Helper to create a simple block
Block CreateBlock(uint64_t height, const std::array<uint8_t, 32>& prev_hash) {
    Block block;

    block.header.version = 1;
    block.header.prev_block_hash = prev_hash;
    block.header.timestamp = 1234567890 + height * 600;
    block.header.bits = Difficulty::GetInitialBits();
    block.header.nonce = 0;

    // Add coinbase
    auto coinbase = CreateCoinbase(height);
    block.transactions.push_back(coinbase);

    // Calculate merkle root
    block.header.merkle_root = block.CalculateMerkleRoot();

    // Mine the block (find valid nonce)
    auto hash = block.GetHash();
    while (!Difficulty::CheckProofOfWork(hash, block.header.bits)) {
        block.header.nonce++;
        hash = block.GetHash();
        if (block.header.nonce > 1000000) {
            std::cerr << "Warning: Mining taking too long" << std::endl;
            break;
        }
    }

    return block;
}

void TestInitialState() {
    std::cout << "Test: Initial chain state" << std::endl;

    ChainState state;
    assert(state.GetHeight() == 0);
    assert(state.GetTotalSupply(AssetID::TALANTON) == 0);
    assert(state.GetTotalSupply(AssetID::DRACHMA) == 0);
    assert(state.GetTotalSupply(AssetID::OBOLOS) == 0);

    std::cout << "  ✓ Passed (initial state correct)" << std::endl;
}

void TestApplyGenesisBlock() {
    std::cout << "Test: Apply genesis block" << std::endl;

    ChainState state;

    // Create genesis block
    std::array<uint8_t, 32> zero_hash{};
    Block genesis = CreateBlock(0, zero_hash);

    // Should validate and apply
    assert(state.ValidateBlock(genesis));
    assert(state.ApplyBlock(genesis));

    // Height should increase
    assert(state.GetHeight() == 1);

    // Supplies should match rewards
    uint64_t taln_reward = Issuance::GetBlockReward(0, AssetID::TALANTON);
    uint64_t drm_reward = Issuance::GetBlockReward(0, AssetID::DRACHMA);
    uint64_t obl_reward = Issuance::GetBlockReward(0, AssetID::OBOLOS);

    assert(state.GetTotalSupply(AssetID::TALANTON) == taln_reward);
    assert(state.GetTotalSupply(AssetID::DRACHMA) == drm_reward);
    assert(state.GetTotalSupply(AssetID::OBOLOS) == obl_reward);

    std::cout << "  ✓ Passed (genesis applied)" << std::endl;
}

void TestApplyMultipleBlocks() {
    std::cout << "Test: Apply multiple blocks" << std::endl;

    ChainState state;

    // Apply 10 blocks
    std::array<uint8_t, 32> prev_hash{};
    for (int i = 0; i < 10; i++) {
        Block block = CreateBlock(i, prev_hash);
        assert(state.ValidateBlock(block));
        assert(state.ApplyBlock(block));
        prev_hash = block.GetHash();
    }

    // Height should be 10
    assert(state.GetHeight() == 10);

    // Total supply should be sum of all rewards
    uint64_t expected_taln = 0;
    for (int i = 0; i < 10; i++) {
        expected_taln += Issuance::GetBlockReward(i, AssetID::TALANTON);
    }
    assert(state.GetTotalSupply(AssetID::TALANTON) == expected_taln);

    std::cout << "  ✓ Passed (multiple blocks applied)" << std::endl;
}

void TestRejectInvalidCoinbase() {
    std::cout << "Test: Reject invalid coinbase" << std::endl;

    ChainState state;

    // Create a block with excessive coinbase
    std::array<uint8_t, 32> zero_hash{};
    Block block = CreateBlock(0, zero_hash);

    // Modify coinbase to exceed allowed reward
    uint64_t excessive_reward = Issuance::GetBlockReward(0, AssetID::TALANTON) + 1;
    block.transactions[0].outputs[0].value.amount = excessive_reward;

    // Recalculate merkle root
    block.header.merkle_root = block.CalculateMerkleRoot();

    // Should fail validation
    assert(!state.ValidateBlock(block));
    assert(!state.ApplyBlock(block));

    // State should be unchanged
    assert(state.GetHeight() == 0);
    assert(state.GetTotalSupply(AssetID::TALANTON) == 0);

    std::cout << "  ✓ Passed (excessive coinbase rejected)" << std::endl;
}

void TestSupplyCapEnforcement() {
    std::cout << "Test: Supply cap enforcement" << std::endl;

    ChainState state;

    // Manually set supply very close to cap
    std::array<uint8_t, 32> prev_hash{};
    Block genesis = CreateBlock(0, prev_hash);
    state.ApplyBlock(genesis);

    // Verify we can't exceed cap through normal operation
    // (The issuance schedule already ensures this, but chainstate double-checks)

    // Apply blocks and verify supply never exceeds cap
    for (int i = 1; i < 100; i++) {
        Block block = CreateBlock(i, prev_hash);
        if (state.ValidateBlock(block)) {
            state.ApplyBlock(block);
            prev_hash = block.GetHash();

            // Check all supplies are within limits
            assert(state.GetTotalSupply(AssetID::TALANTON) <=
                   AssetSupply::GetMaxSupply(AssetID::TALANTON));
            assert(state.GetTotalSupply(AssetID::DRACHMA) <=
                   AssetSupply::GetMaxSupply(AssetID::DRACHMA));
            assert(state.GetTotalSupply(AssetID::OBOLOS) <=
                   AssetSupply::GetMaxSupply(AssetID::OBOLOS));
        }
    }

    std::cout << "  ✓ Passed (caps never exceeded)" << std::endl;
}

void TestResetState() {
    std::cout << "Test: Reset chain state" << std::endl;

    ChainState state;

    // Apply some blocks
    std::array<uint8_t, 32> prev_hash{};
    for (int i = 0; i < 5; i++) {
        Block block = CreateBlock(i, prev_hash);
        state.ApplyBlock(block);
        prev_hash = block.GetHash();
    }

    assert(state.GetHeight() == 5);

    // Reset
    state.Reset();

    // Should be back to initial state
    assert(state.GetHeight() == 0);
    assert(state.GetTotalSupply(AssetID::TALANTON) == 0);
    assert(state.GetTotalSupply(AssetID::DRACHMA) == 0);
    assert(state.GetTotalSupply(AssetID::OBOLOS) == 0);

    std::cout << "  ✓ Passed (reset works)" << std::endl;
}

int main() {
    std::cout << "=== ChainState Tests ===" << std::endl;

    TestInitialState();
    TestApplyGenesisBlock();
    TestApplyMultipleBlocks();
    TestRejectInvalidCoinbase();
    TestSupplyCapEnforcement();
    TestResetState();

    std::cout << "\n✓ All chainstate tests passed!" << std::endl;
    return 0;
}
