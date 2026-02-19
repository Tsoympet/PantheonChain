#include "common/commitments.h"
#include "common/cryptography.h"
#include "common/mempool.h"
#include "common/metrics.h"
#include "common/p2p_network.h"
#include "common/serialization.h"
#include "common/storage.h"
#include "drachma/pos_consensus.h"
#include "obolos/execution.h"
#include "talanton/l1_commitment_validator.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace pantheon;

int main() {
    common::Commitment l3_commit{
        common::SourceChain::OBOLOS,
        1,
        10,
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
        {{"val1", 70, "sig1"}, {"val2", 40, "sig2"}}};

    auto l3_valid = drachma::ValidateL3Commit(l3_commit, 5, 150);
    assert(l3_valid.valid);

    common::Commitment l2_commit = l3_commit;
    l2_commit.source_chain = common::SourceChain::DRACHMA;
    auto l2_valid = talanton::ValidateL2Commit(l2_commit, {9}, 150);
    assert(l2_valid.valid);

    const std::vector<drachma::Validator> validators{{"val1", 80}, {"val2", 20}};
    const auto& proposer_a = drachma::SelectDeterministicProposer(validators, 3, 22);
    const auto& proposer_b = drachma::SelectDeterministicProposer(validators, 3, 22);
    assert(proposer_a.id == proposer_b.id);

    auto slash_double_sign = drachma::SlashDoubleSign(validators.front(), 1, 20);
    assert(slash_double_sign.slashed_amount == 4);
    auto slash_equivocation = drachma::SlashEquivocation(validators.front(), 1, 10);
    assert(slash_equivocation.slashed_amount == 8);

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
