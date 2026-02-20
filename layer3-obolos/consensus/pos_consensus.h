#pragma once

#include "common/commitments.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::obolos {

struct Validator {
    std::string id;
    uint64_t stake = 0;
};

uint64_t TotalActiveStake(const std::vector<Validator>& validators);

const Validator& SelectDeterministicProposer(const std::vector<Validator>& validators,
                                             uint64_t epoch,
                                             uint64_t height);

common::Commitment BuildL3Commitment(uint64_t epoch,
                                     uint64_t finalized_height,
                                     const std::string& finalized_block_hash,
                                     const std::string& state_root,
                                     const std::string& validator_set_hash,
                                     std::vector<common::FinalitySignature> signatures);

common::CommitmentValidationResult ValidateL3Finality(const common::Commitment& commitment,
                                                      uint64_t last_finalized_height,
                                                      uint64_t active_stake);

}  // namespace pantheon::obolos
