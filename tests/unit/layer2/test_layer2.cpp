#include "layer2/apis/graphql/graphql_api.h"
#include "layer2/apis/websocket/websocket_api.h"
#include "layer2/bridges/channels/payment_channel.h"
#include "layer2/bridges/htlc/htlc.h"
#include "layer2/bridges/spv/spv_bridge.h"
#include "layer2/plasma/plasma_chain.h"
#include "layer2/rollups/optimistic_rollup.h"
#include "layer2/rollups/zk_rollup.h"
#include "privacy/zk_snark.h"

#include <cassert>
#include <iostream>

using namespace parthenon::layer2;

namespace {

std::vector<uint8_t> BuildExitInputs(const std::vector<uint8_t> &account, uint64_t amount) {
    std::vector<uint8_t> inputs = account;
    const auto *bytes = reinterpret_cast<const uint8_t *>(&amount);
    inputs.insert(inputs.end(), bytes, bytes + sizeof(uint64_t));
    return inputs;
}

class ExitCircuit : public parthenon::privacy::zksnark::Circuit {
  public:
    explicit ExitCircuit(std::vector<uint8_t> inputs) : inputs_(std::move(inputs)) {}

    size_t GetConstraintCount() const override { return inputs_.size() + 1; }
    size_t GetInputCount() const override { return inputs_.size(); }
    bool Synthesize() override { return !inputs_.empty(); }

  private:
    std::vector<uint8_t> inputs_;
};

} // namespace

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
    assert(channel.InitiateClose(86400)); // 1 day dispute period
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

void test_optimistic_rollup_and_plasma() {
    std::cout << "Testing optimistic rollup and plasma..." << std::endl;

    rollups::OptimisticRollup rollup;
    rollups::RollupTx tx;
    tx.from = std::vector<uint8_t>(32, 0x01);
    tx.to = std::vector<uint8_t>(32, 0x02);
    tx.signature = std::vector<uint8_t>(64, 0x03);
    tx.tx_hash[0] = 0xAA;
    assert(rollup.AddTransaction(tx));

    auto batch = rollup.CreateBatch();
    batch.state_root_after[0] = 0x55;
    batch.batch_id = 1;
    assert(rollup.SubmitBatch(batch));
    assert(rollup.GetBatch(1).has_value());

    auto compressed = rollup.CompressBatch(batch);
    auto decompressed = rollup.DecompressBatch(compressed);
    assert(decompressed.has_value());

    plasma::PlasmaChain chain;
    plasma::PlasmaOperator op(&chain);
    plasma::PlasmaTx ptx;
    ptx.sender = std::vector<uint8_t>(32, 0x0A);
    ptx.recipient = std::vector<uint8_t>(32, 0x0B);
    ptx.amount = 100;
    ptx.signature = std::vector<uint8_t>(64, 0x0C);
    ptx.tx_hash[0] = 0x22;
    assert(chain.AddTransaction(ptx));

    auto block = op.CreateBlock();
    assert(chain.SubmitBlock(block));
    assert(chain.GetBlock(block.block_number).has_value());

    std::cout << "Optimistic rollup/plasma tests passed!" << std::endl;
}

void test_rollup_lifecycle() {
    std::cout << "Testing rollup lifecycle..." << std::endl;

    rollups::OptimisticRollup rollup;
    rollup.SetChallengePeriod(0);

    rollups::RollupTx tx;
    tx.from = std::vector<uint8_t>(32, 0x11);
    tx.to = std::vector<uint8_t>(32, 0x22);
    tx.signature = std::vector<uint8_t>(64, 0x33);
    tx.tx_hash[0] = 0x44;
    assert(rollup.AddTransaction(tx));

    auto batch = rollup.CreateBatch();
    batch.state_root_after[0] = 0x55;
    batch.batch_id = 1;
    assert(rollup.SubmitBatch(batch));

    rollups::FraudProof proof;
    proof.batch_id = 1;
    proof.disputed_tx_index = 0;
    proof.claimed_state_root = batch.state_root_after;
    proof.correct_state_root = batch.state_root_before;
    proof.witness_data = {0x01, 0x02};
    assert(rollup.SubmitFraudProof(proof));
    assert(!rollup.FinalizeBatch(1));

    rollups::RollupTx tx2;
    tx2.from = std::vector<uint8_t>(32, 0x44);
    tx2.to = std::vector<uint8_t>(32, 0x55);
    tx2.signature = std::vector<uint8_t>(64, 0x66);
    tx2.tx_hash[0] = 0x77;
    assert(rollup.AddTransaction(tx2));

    auto batch2 = rollup.CreateBatch();
    batch2.state_root_after[0] = 0x88;
    assert(rollup.SubmitBatch(batch2));
    assert(rollup.FinalizeBatch(batch2.batch_id));

    std::cout << "Rollup lifecycle tests passed!" << std::endl;
}

void test_zk_rollup_lifecycle_and_exit() {
    std::cout << "Testing ZK-rollup lifecycle and exit..." << std::endl;

    constexpr uint64_t kExitAmount = 25;

    rollups::ZKRollup rollup;
    rollups::ZKRollupProver prover;
    rollups::ZKRollupVerifier verifier(&rollup);

    rollups::ZKTransaction tx;
    tx.tx_hash[0] = 0x10;
    tx.nullifier[0] = 0x20;
    tx.commitment[0] = 0x30;
    tx.transfer_proof = prover.GenerateTransferProof(tx, {});
    assert(rollup.AddTransaction(tx));

    auto batch = rollup.CreateBatch();
    batch.validity_proof = prover.GenerateBatchProof(batch);
    assert(rollup.SubmitBatch(batch));
    assert(verifier.VerifyBatchProof(batch));
    assert(rollup.FinalizeBatch(batch.batch_id));

    auto compressed = rollup.CompressBatch(batch);
    auto decompressed = rollup.DecompressBatch(compressed);
    assert(decompressed.has_value());
    assert(decompressed->batch_id == batch.batch_id);
    assert(decompressed->transaction_hashes.size() == batch.transaction_hashes.size());

    rollups::ZKRollupState state;
    assert(state.ApplyTransaction(tx));

    std::vector<uint8_t> account(tx.nullifier.begin(), tx.nullifier.end());
    auto merkle_proof = state.GetMerkleProof(account);
    auto merkle_root = state.GetStateRoot();

    auto exit_inputs = BuildExitInputs(account, kExitAmount);
    parthenon::privacy::zksnark::ZKProver exit_prover(rollup.GetProofParameters());
    ExitCircuit circuit(exit_inputs);
    auto proof_opt = exit_prover.GenerateProof(circuit, exit_inputs);
    assert(proof_opt.has_value());

    rollups::ZKRollupExitManager exit_manager(rollup.GetProofParameters());
    rollups::ZKRollupExitManager::ExitRequest request;
    request.account = account;
    request.amount = kExitAmount;
    request.merkle_root = merkle_root;
    request.merkle_proof = merkle_proof;
    request.ownership_proof = *proof_opt;
    assert(exit_manager.RequestExit(request));
    assert(exit_manager.ProcessExit(account));

    std::cout << "ZK-rollup lifecycle/exit tests passed!" << std::endl;
}

void test_layer2_apis() {
    std::cout << "Testing Layer 2 API servers..." << std::endl;

    apis::GraphQLAPI graphql_api(8080);
    assert(!graphql_api.IsRunning());
    assert(graphql_api.Start());
    assert(!graphql_api.Start());
    assert(graphql_api.IsRunning());
    graphql_api.Stop();
    assert(!graphql_api.IsRunning());
    assert(graphql_api.Start());
    assert(graphql_api.IsRunning());
    graphql_api.Stop();
    assert(!graphql_api.IsRunning());

    apis::GraphQLAPI invalid_graphql_api(0);
    assert(!invalid_graphql_api.Start());
    assert(!invalid_graphql_api.IsRunning());

    apis::WebSocketAPI websocket_api(8081);
    assert(!websocket_api.IsRunning());
    assert(websocket_api.Start());
    assert(!websocket_api.Start());
    assert(websocket_api.IsRunning());
    std::vector<std::string> sent_messages;
    websocket_api.SetSendHandler([&sent_messages](void *connection, const std::string &message) {
        static_cast<void>(connection);
        sent_messages.push_back(message);
    });
    const uint64_t test_client_id = 1;
    websocket_api.Subscribe(test_client_id, "blocks");
    assert(websocket_api.GetConnectedClients() == 1);
    assert(websocket_api.GetSubscriptionCount("blocks") == 1);
    websocket_api.Broadcast("ping");
    assert(websocket_api.GetLastBroadcastMessage() == "ping");
    assert(sent_messages.size() == 1);
    assert(sent_messages.back() == "ping");
    websocket_api.PublishToTopic("blocks", "block-1");
    assert(websocket_api.GetLastTopicMessage("blocks") == "block-1");
    assert(sent_messages.size() == 2);
    assert(sent_messages.back() == "block-1");
    websocket_api.Stop();
    assert(websocket_api.GetConnectedClients() == 0);
    assert(websocket_api.GetSubscriptionCount("blocks") == 0);
    assert(!websocket_api.IsRunning());
    assert(websocket_api.Start());
    assert(websocket_api.IsRunning());
    websocket_api.Stop();
    assert(!websocket_api.IsRunning());

    apis::WebSocketAPI invalid_websocket_api(0);
    assert(!invalid_websocket_api.Start());
    assert(!invalid_websocket_api.IsRunning());

    std::cout << "Layer 2 API server tests passed!" << std::endl;
}

int main() {
    try {
        test_payment_channel();
        test_htlc();
        test_htlc_routing();
        test_spv_merkle_proof();
        test_optimistic_rollup_and_plasma();
        test_rollup_lifecycle();
        test_zk_rollup_lifecycle_and_exit();
        test_layer2_apis();

        std::cout << "\n✓ All Layer 2 tests passed!" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
