#include "common/commitments.h"
#include "layer1-talanton/tx/l1_commitment_validator.h"
#include "layer2-drachma/consensus/pos_consensus.h"
#include "layer3-obolos/consensus/pos_consensus.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace pantheon;

    common::Commitment valid_l3 = obolos::BuildL3Commitment(
        7,
        101,
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
        {{"val1", 67, "sig-a"}, {"val2", 34, "sig-b"}});

    auto l3_ok = obolos::ValidateL3Finality(valid_l3, 100, 150);
    assert(l3_ok.valid);

    // Adversarial: replay / non-monotonic height should fail.
    auto l3_replay = obolos::ValidateL3Finality(valid_l3, 101, 150);
    assert(!l3_replay.valid);

    // Adversarial: insufficient quorum should fail.
    common::Commitment weak_l3 = valid_l3;
    weak_l3.signatures = {{"val1", 40, "sig-a"}};
    auto l3_quorum_fail = obolos::ValidateL3Finality(weak_l3, 100, 150);
    assert(!l3_quorum_fail.valid);

    // Build DRACHMA commitment that carries L3 upstream anchor.
    common::Commitment l2_commit{
        common::SourceChain::DRACHMA,
        9,
        202,
        "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "9999999999999999999999999999999999999999999999999999999999999999",
        {{"val3", 80, "sig-c"}, {"val4", 30, "sig-d"}},
    };

    auto l2_ok = talanton::ValidateL2Commit(l2_commit, {201}, 160);
    assert(l2_ok.valid);

    // Adversarial: forged source chain should be rejected.
    common::Commitment forged = l2_commit;
    forged.source_chain = common::SourceChain::OBOLOS;
    auto l2_forged = talanton::ValidateL2Commit(forged, {201}, 160);
    assert(!l2_forged.valid);

    // Slashing path sanity: equivocation materially reduces stake.
    drachma::Validator validator{"bad-val", 100000};
    auto slash = drachma::ApplySlashing(validator, "equivocation");
    assert(slash.slashed_amount == 10000);
    assert(validator.stake == 90000);

    std::cout << "Adversarial commitment tests passed" << std::endl;
    return 0;
}
