#include "common/commitments.h"
#include "common/cryptography.h"
#include "common/mempool.h"
#include "common/metrics.h"
#include "common/p2p_network.h"
#include "common/serialization.h"
#include "common/storage.h"
#include "drachma/payments_state_machine.h"
#include "drachma/pos_consensus.h"
#include "obolos/execution.h"
#include "obolos/pos_consensus.h"
#include "talanton/l1_commitment_validator.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace pantheon;

int main() {
    // OBOLOS L3 finality commitment
    common::Commitment l3_commit = obolos::BuildL3Commitment(
        1, 10, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
        {{"val1", 70, "sig1"}, {"val2", 40, "sig2"}});

    auto l3_valid = obolos::ValidateL3Finality(l3_commit, 5, 150);
    assert(l3_valid.valid);

    // DRACHMA anchors L3 hash inside L2 state root material.
    common::Commitment l2_commit{
        common::SourceChain::DRACHMA,
        1,
        10,
        "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
        common::PseudoSha256d(l3_commit.state_root + l3_commit.finalized_block_hash),
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        {{"val1", 70, "sig1"}, {"val2", 40, "sig2"}}};
    auto l2_valid = talanton::ValidateL2Commit(l2_commit, {9}, 150);
    assert(l2_valid.valid);

    const std::vector<drachma::Validator> l2_validators{{"val1", 80}, {"val2", 20}};
    const auto& l2_proposer_a = drachma::SelectDeterministicProposer(l2_validators, 3, 22);
    const auto& l2_proposer_b = drachma::SelectDeterministicProposer(l2_validators, 3, 22);
    assert(l2_proposer_a.id == l2_proposer_b.id);

    const std::vector<obolos::Validator> l3_validators{{"val3", 60}, {"val4", 40}};
    const auto& l3_proposer_a = obolos::SelectDeterministicProposer(l3_validators, 7, 99);
    const auto& l3_proposer_b = obolos::SelectDeterministicProposer(l3_validators, 7, 99);
    assert(l3_proposer_a.id == l3_proposer_b.id);

    auto slash_double_sign = drachma::SlashDoubleSign(l2_validators.front(), 1, 20);
    assert(slash_double_sign.slashed_amount == 4);
    auto slash_equivocation = drachma::SlashEquivocation(l2_validators.front(), 1, 10);
    assert(slash_equivocation.slashed_amount == 8);

    drachma::PaymentsStateMachine payments;
    payments.Credit("alice", 1000);
    auto transfer = payments.Transfer("alice", "bob", 250, 5);
    assert(transfer.ok);
    assert(payments.Balance("alice") == 745);
    assert(payments.Balance("bob") == 250);
    assert(payments.CollectedFees() == 5);

    auto exec_ok = obolos::ExecuteEvmLikeCall("6001600055", 50000, 1);
    assert(exec_ok.success);

    auto exec_fail = obolos::ExecuteEvmLikeCall("6001600055", 1000, 1);
    assert(!exec_fail.success);

    common::Mempool mempool;
    mempool.Add("tx-1");
    mempool.Add("tx-1");
    mempool.Add("tx-2");
    assert(mempool.Size() == 2);
    assert(mempool.PopFront() == "tx-1");

    common::MetricsRegistry metrics;
    metrics.Increment("commitments.accepted");
    metrics.Increment("commitments.accepted", 2);
    assert(metrics.Read("commitments.accepted") == 3);

    common::KeyValueStorage storage;
    storage.Put("latest_l2_root", l2_commit.state_root);
    assert(storage.Get("latest_l2_root").has_value());

    common::P2PNetwork network;
    network.Broadcast({"commitment", common::EncodeCommitment(l2_commit)});
    assert(network.Outbox().size() == 1);

    const auto digest = common::PseudoSha256d(common::EncodeCommitment(l3_commit));
    assert(digest.size() == 64);

    std::cout << "Layered architecture consensus tests passed" << std::endl;
    return 0;
}
