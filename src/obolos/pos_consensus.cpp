#include "pos_consensus.h"

#include <stdexcept>

namespace pantheon::obolos {

uint64_t TotalActiveStake(const std::vector<Validator>& validators) {
    uint64_t total = 0;
    for (const auto& validator : validators) {
        total += validator.stake;
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

    const uint64_t slot = (epoch << 32U) ^ height;
    uint64_t cursor = slot % total_stake;
    for (const auto& validator : validators) {
        if (cursor < validator.stake) {
            return validator;
        }
        cursor -= validator.stake;
    }

    return validators.back();
}

common::Commitment BuildL3Commitment(uint64_t epoch, uint64_t finalized_height,
                                     const std::string& finalized_block_hash,
                                     const std::string& state_root,
                                     const std::string& validator_set_hash,
                                     std::vector<common::FinalitySignature> signatures) {
    return {common::SourceChain::OBOLOS, epoch, finalized_height, finalized_block_hash, state_root,
            validator_set_hash, std::move(signatures)};
}

common::CommitmentValidationResult ValidateL3Finality(const common::Commitment& commitment,
                                                      uint64_t last_finalized_height,
                                                      uint64_t active_stake) {
    if (commitment.source_chain != common::SourceChain::OBOLOS) {
        return {false, "L3 finality payload must originate from OBOLOS"};
    }

    if (commitment.finalized_height <= last_finalized_height) {
        return {false, "L3 finalized_height must be monotonic"};
    }

    auto encoding = common::ValidatePayloadEncoding(commitment);
    if (!encoding.valid) {
        return encoding;
    }

    return common::ValidateFinalityQuorum(commitment, active_stake);
}

}  // namespace pantheon::obolos
