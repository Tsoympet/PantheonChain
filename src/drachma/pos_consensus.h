#pragma once

#include "common/commitments.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::drachma {

struct Validator {
    std::string id;
    uint64_t stake = 0;
};

struct SlashingEvent {
    std::string validator_id;
    std::string reason;
    uint64_t slashed_amount = 0;
};

uint64_t TotalActiveStake(const std::vector<Validator>& validators);

const Validator& SelectDeterministicProposer(const std::vector<Validator>& validators,
                                             uint64_t epoch,
                                             uint64_t height);

SlashingEvent SlashDoubleSign(const Validator& validator, uint64_t ratio_numerator,
                              uint64_t ratio_denominator);

common::CommitmentValidationResult ValidateL3Commit(const common::Commitment& commitment,
                                                    uint64_t last_l3_height,
                                                    uint64_t active_stake);

}  // namespace pantheon::drachma
