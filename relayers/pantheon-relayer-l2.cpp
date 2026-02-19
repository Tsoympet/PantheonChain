#include "common/commitments.h"
#include "common/serialization.h"
#include "talanton/l1_commitment_validator.h"

#include <iostream>

int main() {
    using namespace pantheon;

    common::Commitment commit{
        common::SourceChain::DRACHMA,
        8,
        120,
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
        {{"l2-val-1", 45, "sig-a"}, {"l2-val-2", 35, "sig-b"}}};

    talanton::L2AnchorState anchor_state{};
    anchor_state.last_finalized_height = 100;
    auto result = talanton::ValidateL2Commit(commit, anchor_state, 100);
    if (!result.valid) {
        std::cerr << "pantheon-relayer-l2 rejected commitment: " << result.reason << std::endl;
        return 1;
    }

    std::cout << "pantheon-relayer-l2 relayed: " << common::EncodeCommitment(commit) << std::endl;
    return 0;
}
