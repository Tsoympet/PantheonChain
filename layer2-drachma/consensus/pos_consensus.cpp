// PantheonChain — Layer-2 DRACHMA PoS/BFT Consensus Implementation

#include "pos_consensus.h"

#include <stdexcept>

namespace pantheon::drachma {

uint64_t TotalActiveStake(const std::vector<Validator>& validators) {
    uint64_t total = 0;
    for (const auto& v : validators) {
        total += v.stake;
    }
    return total;
}

const Validator& SelectDeterministicProposer(const std::vector<Validator>& validators,
                                              uint64_t epoch,
                                              uint64_t height) {
    if (validators.empty()) {
        throw std::invalid_argument("validators cannot be empty");
    }

    const uint64_t total_stake = TotalActiveStake(validators);
    if (total_stake == 0) {
        throw std::invalid_argument("total active stake cannot be zero");
    }

    // Deterministic slot assignment: XOR epoch/height, then pick by stake range.
    const uint64_t slot = (epoch << 32U) ^ height;
    uint64_t cursor = slot % total_stake;
    for (const auto& v : validators) {
        if (cursor < v.stake) {
            return v;
        }
        cursor -= v.stake;
    }

    return validators.back();
}

common::CommitmentValidationResult ValidateL3Commit(const common::Commitment& commitment,
                                                    uint64_t last_l3_height,
                                                    uint64_t active_stake) {
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

    // Quorum check: >=2/3 of contributing stake must have signed.
    return common::ValidateFinalityQuorum(commitment, active_stake);
}

}  // namespace pantheon::drachma
