#include "common/commitments.h"
#include "common/serialization.h"
#include "drachma/pos_consensus.h"

#include <iostream>

int main() {
    using namespace pantheon;

    common::Commitment commit{
        common::SourceChain::OBOLOS,
        18,
        640,
        "1111111111111111111111111111111111111111111111111111111111111111",
        "2222222222222222222222222222222222222222222222222222222222222222",
        "3333333333333333333333333333333333333333333333333333333333333333",
        {{"l3-val-1", 50, "sig-a"}, {"l3-val-2", 30, "sig-b"}}};

    auto result = drachma::ValidateL3Commit(commit, 500, 100);
    if (!result.valid) {
        std::cerr << "pantheon-relayer-l3 rejected commitment: " << result.reason << std::endl;
        return 1;
    }

    std::cout << "pantheon-relayer-l3 relayed: " << common::EncodeCommitment(commit) << std::endl;
    return 0;
}
