// ParthenonChain - Chain Tests
// Test block connection, disconnection, and reorg handling

#include "chainstate/chain.h"
#include "consensus/difficulty.h"
#include "consensus/issuance.h"

#include <cassert>
#include <iostream>

using namespace parthenon::chainstate;
using namespace parthenon::primitives;
using namespace parthenon::consensus;

// Helper to create a simple coinbase transaction
Transaction CreateCoinbase(uint64_t height) {
    Transaction tx;

    // Coinbase input
    TxInput coinbase_input;
    coinbase_input.prevout.txid = std::array<uint8_t, 32>{};
    coinbase_input.prevout.vout = COINBASE_VOUT_INDEX;
    tx.inputs.push_back(coinbase_input);

    // Coinbase outputs
    std::vector<uint8_t> pubkey(32, 0xAB);

    uint64_t taln_reward = Issuance::GetBlockReward(height, AssetID::TALANTON);
    uint64_t drm_reward = Issuance::GetBlockReward(height, AssetID::DRACHMA);
    uint64_t obl_reward = Issuance::GetBlockReward(height, AssetID::OBOLOS);

    if (taln_reward > 0) {
        tx.outputs.push_back(TxOutput(AssetID::TALANTON, taln_reward, pubkey));
    }
    if (drm_reward > 0) {
        tx.outputs.push_back(TxOutput(AssetID::DRACHMA, drm_reward, pubkey));
    }
    if (obl_reward > 0) {
        tx.outputs.push_back(TxOutput(AssetID::OBOLOS, obl_reward, pubkey));
    }

    return tx;
}

// Helper to mine a block
Block CreateAndMineBlock(uint64_t height, const std::array<uint8_t, 32>& prev_hash) {
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

    // Mine
    while (!block.header.MeetsDifficultyTarget()) {
        block.header.nonce++;
        if (block.header.nonce > 10000000) {
            break;
        }
    }

    return block;
}

void TestChainInitialization() {
    std::cout << "Test: Chain initialization" << std::endl;

    Chain chain;

    assert(chain.GetHeight() == 0);
    assert(chain.GetUTXOSet().GetSize() == 0);
    assert(chain.GetTotalSupply(AssetID::TALANTON) == 0);
    assert(chain.GetTotalSupply(AssetID::DRACHMA) == 0);
    assert(chain.GetTotalSupply(AssetID::OBOLOS) == 0);

    std::cout << "  ✓ Passed (initial state)" << std::endl;
}

void TestConnectGenesisBlock() {
    std::cout << "Test: Connect genesis block" << std::endl;

    Chain chain;

    // Create genesis block
    std::array<uint8_t, 32> zero_hash{};
    Block genesis = CreateAndMineBlock(0, zero_hash);

    // Connect block
    BlockUndo undo;
    assert(chain.ConnectBlock(genesis, undo));

    // Verify state
    assert(chain.GetHeight() == 1);
    assert(chain.GetTip() == genesis.GetHash());

    // Verify UTXO set contains coinbase outputs
    assert(chain.GetUTXOSet().GetSize() == genesis.transactions[0].outputs.size());

    // Verify supply tracking
    uint64_t taln_reward = Issuance::GetBlockReward(0, AssetID::TALANTON);
    uint64_t drm_reward = Issuance::GetBlockReward(0, AssetID::DRACHMA);
    uint64_t obl_reward = Issuance::GetBlockReward(0, AssetID::OBOLOS);

    assert(chain.GetTotalSupply(AssetID::TALANTON) == taln_reward);
    assert(chain.GetTotalSupply(AssetID::DRACHMA) == drm_reward);
    assert(chain.GetTotalSupply(AssetID::OBOLOS) == obl_reward);

    std::cout << "  ✓ Passed (genesis connected)" << std::endl;
}

void TestConnectMultipleBlocks() {
    std::cout << "Test: Connect multiple blocks" << std::endl;

    Chain chain;

    // Connect 5 blocks
    std::array<uint8_t, 32> prev_hash{};
    for (int i = 0; i < 5; i++) {
        Block block = CreateAndMineBlock(i, prev_hash);

        BlockUndo undo;
        assert(chain.ConnectBlock(block, undo));

        prev_hash = block.GetHash();
    }

    // Verify state
    assert(chain.GetHeight() == 5);

    // Verify total supply accumulated
    uint64_t expected_taln = 0;
    for (int i = 0; i < 5; i++) {
        expected_taln += Issuance::GetBlockReward(i, AssetID::TALANTON);
    }
    assert(chain.GetTotalSupply(AssetID::TALANTON) == expected_taln);

    std::cout << "  ✓ Passed (multiple blocks)" << std::endl;
}

void TestDisconnectBlock() {
    std::cout << "Test: Disconnect block" << std::endl;

    Chain chain;

    // Connect 3 blocks
    std::array<uint8_t, 32> prev_hash{};
    std::vector<Block> blocks;
    std::vector<BlockUndo> undos;

    for (int i = 0; i < 3; i++) {
        Block block = CreateAndMineBlock(i, prev_hash);
        blocks.push_back(block);

        BlockUndo undo;
        assert(chain.ConnectBlock(block, undo));
        undos.push_back(undo);

        prev_hash = block.GetHash();
    }

    assert(chain.GetHeight() == 3);

    // Disconnect last block
    assert(chain.DisconnectBlock(blocks[2], undos[2]));
    assert(chain.GetHeight() == 2);
    assert(chain.GetTip() == blocks[1].GetHash());

    // Verify supply was reverted
    uint64_t expected_taln = 0;
    for (int i = 0; i < 2; i++) {
        expected_taln += Issuance::GetBlockReward(i, AssetID::TALANTON);
    }
    assert(chain.GetTotalSupply(AssetID::TALANTON) == expected_taln);

    std::cout << "  ✓ Passed (disconnect works)" << std::endl;
}

void TestCannotDisconnectGenesis() {
    std::cout << "Test: Cannot disconnect when height is 0" << std::endl;

    Chain chain;

    // Create a block (but don't connect it)
    std::array<uint8_t, 32> zero_hash{};
    Block block = CreateAndMineBlock(0, zero_hash);

    BlockUndo undo;

    // Try to disconnect (should fail)
    assert(!chain.DisconnectBlock(block, undo));
    assert(chain.GetHeight() == 0);

    std::cout << "  ✓ Passed (genesis protection)" << std::endl;
}

void TestReset() {
    std::cout << "Test: Reset chain" << std::endl;

    Chain chain;

    // Connect some blocks
    std::array<uint8_t, 32> prev_hash{};
    for (int i = 0; i < 3; i++) {
        Block block = CreateAndMineBlock(i, prev_hash);

        BlockUndo undo;
        chain.ConnectBlock(block, undo);

        prev_hash = block.GetHash();
    }

    assert(chain.GetHeight() == 3);
    assert(chain.GetUTXOSet().GetSize() > 0);

    // Reset
    chain.Reset();

    // Verify reset
    assert(chain.GetHeight() == 0);
    assert(chain.GetUTXOSet().GetSize() == 0);
    assert(chain.GetTotalSupply(AssetID::TALANTON) == 0);
    assert(chain.GetTotalSupply(AssetID::DRACHMA) == 0);
    assert(chain.GetTotalSupply(AssetID::OBOLOS) == 0);

    std::cout << "  ✓ Passed (reset works)" << std::endl;
}

int main() {
    std::cout << "=== Chain Tests ===" << std::endl;

    TestChainInitialization();
    TestConnectGenesisBlock();
    TestConnectMultipleBlocks();
    TestDisconnectBlock();
    TestCannotDisconnectGenesis();
    TestReset();

    std::cout << "\n✓ All chain tests passed!" << std::endl;
    return 0;
}
