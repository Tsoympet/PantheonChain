#include "common/serialization.h"
#include "drachma/pos_consensus.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

std::string RandomHex(std::mt19937_64& rng, std::size_t len) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(kHex[rng() % 16]);
    }
    return out;
}

std::string RandomMaybeMalformedCommitment(std::mt19937_64& rng) {
    const bool valid = (rng() % 3) == 0;
    if (!valid) {
        return RandomHex(rng, static_cast<std::size_t>(rng() % 60 + 1));
    }

    pantheon::common::Commitment commitment{
        (rng() % 2) == 0 ? pantheon::common::SourceChain::DRACHMA
                         : pantheon::common::SourceChain::OBOLOS,
        rng() % 100,
        (rng() % 1000) + 1,
        RandomHex(rng, 64),
        RandomHex(rng, 64),
        RandomHex(rng, 64),
        {{"v1", 70, "sig1"}, {"v2", 40, "sig2"}}};
    return pantheon::common::EncodeCommitment(commitment);
}

}  // namespace

int main() {
    std::mt19937_64 rng(1337);

    // Fuzz commitment decoding path.
    for (int i = 0; i < 5000; ++i) {
        pantheon::common::Commitment decoded{};
        const auto payload = RandomMaybeMalformedCommitment(rng);
        const auto result = pantheon::common::DecodeCommitment(payload, decoded);
        if (result.valid) {
            assert(decoded.finalized_height > 0);
            assert(decoded.signatures.size() >= 1);
        }
    }

    // Fuzz proposer determinism under random stakes.
    for (int i = 0; i < 1000; ++i) {
        std::vector<pantheon::drachma::Validator> validators;
        validators.push_back({"a", (rng() % 100) + 1});
        validators.push_back({"b", (rng() % 100) + 1});
        validators.push_back({"c", (rng() % 100) + 1});

        const uint64_t epoch = rng() % 1024;
        const uint64_t height = rng() % 100000;
        const auto& p1 = pantheon::drachma::SelectDeterministicProposer(validators, epoch, height);
        const auto& p2 = pantheon::drachma::SelectDeterministicProposer(validators, epoch, height);
        assert(p1.id == p2.id);
    }

    std::cout << "Layer fuzzing tests passed" << std::endl;
    return 0;
}
