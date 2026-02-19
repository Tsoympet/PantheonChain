#include "common/serialization.h"

#include <cassert>
#include <iostream>

int main() {
    pantheon::common::Commitment commitment{
        pantheon::common::SourceChain::DRACHMA,
        2,
        42,
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
        "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
        {{"val1", 70, "sig-a"}, {"val2", 40, "sig-b"}}};

    const std::string encoded = pantheon::common::EncodeCommitment(commitment);

    pantheon::common::Commitment decoded{};
    auto decode_result = pantheon::common::DecodeCommitment(encoded, decoded);
    assert(decode_result.valid);
    assert(decoded.source_chain == pantheon::common::SourceChain::DRACHMA);
    assert(decoded.epoch == 2);
    assert(decoded.finalized_height == 42);
    assert(decoded.finalized_block_hash == commitment.finalized_block_hash);
    assert(decoded.signatures.size() == 2);
    assert(decoded.signatures[0].validator_id == "val1");
    assert(decoded.upstream_commitment_hash ==
           "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");

    auto bad_result =
        pantheon::common::DecodeCommitment("DRACHMA:abc:42:block:state:validators:upstream:sigs", decoded);
    assert(!bad_result.valid);

    auto missing_upstream = pantheon::common::DecodeCommitment(
        "DRACHMA:2:42:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:"
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc::"
        "val1|70|sig-a",
        decoded);
    assert(!missing_upstream.valid);

    std::cout << "Commitment serialization tests passed" << std::endl;
    return 0;
}
