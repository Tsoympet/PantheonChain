// PantheonChain — Layer-2 DRACHMA PoW Consensus Implementation

#include "pos_consensus.h"

#include <stdexcept>

namespace pantheon::drachma {

uint64_t TotalHashPower(const std::vector<Miner>& miners) {
    uint64_t total = 0;
    for (const auto& miner : miners) {
        total += miner.hash_power;
    }
    return total;
}

const Miner& SelectMiner(const std::vector<Miner>& miners,
                         uint64_t epoch,
                         uint64_t height) {
    if (miners.empty()) {
        throw std::invalid_argument("miners cannot be empty");
    }

    const uint64_t total_pow = TotalHashPower(miners);
    if (total_pow == 0) {
        throw std::invalid_argument("total hash power cannot be zero");
    }

    // Deterministic slot assignment: XOR epoch/height, then pick by hash-power range.
    const uint64_t slot = (epoch << 32U) ^ height;
    uint64_t cursor = slot % total_pow;
    for (const auto& miner : miners) {
        if (cursor < miner.hash_power) {
            return miner;
        }
        cursor -= miner.hash_power;
    }

    return miners.back();
}

common::CommitmentValidationResult ValidateL3Commit(const common::Commitment& commitment,
                                                    uint64_t last_l3_height,
                                                    uint64_t active_pow) {
    if (commitment.source_chain != common::SourceChain::OBOLOS) {
        return {false, "TX_L3_COMMIT must originate from OBOLOS"};
    }
    if (commitment.finalized_height <= last_l3_height) {
        return {false, "TX_L3_COMMIT finalized_height must be monotonic"};
    }

    auto encoding = common::ValidatePayloadEncoding(commitment);
    if (!encoding.valid) {
        return encoding;
    }

    // Quorum check: >=2/3 of contributing hash power must have signed.
    return common::ValidateFinalityQuorum(commitment, active_pow);
}

}  // namespace pantheon::drachma
