#include "common/commitments.h"
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

    auto slash = drachma::SlashDoubleSign(validators.front(), 1, 20);
    assert(slash.slashed_amount == 4);

    auto exec_ok = obolos::ExecuteEvmLikeCall("6001600055", 50000, 1);
    assert(exec_ok.success);

    auto exec_fail = obolos::ExecuteEvmLikeCall("6001600055", 1000, 1);
    assert(!exec_fail.success);

    std::cout << "Layered architecture consensus tests passed" << std::endl;
    return 0;
}
