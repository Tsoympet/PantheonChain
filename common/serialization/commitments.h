#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::common {

enum class SourceChain {
    DRACHMA,
    OBOLOS,
};

struct FinalitySignature {
    std::string validator_id;
    uint64_t stake_weight = 0;
    std::string signature;
};

struct Commitment {
    SourceChain source_chain;
    uint64_t epoch = 0;
    uint64_t finalized_height = 0;
    std::string finalized_block_hash;
    std::string state_root;
    std::string validator_set_hash;
    // For DRACHMA -> TALANTON commitments this carries the latest finalized
    // OBOLOS commitment hash to preserve the canonical anchoring chain
    // OBOLOS -> DRACHMA -> TALANTON.
    std::string upstream_commitment_hash;
    std::vector<FinalitySignature> signatures;
};

struct CommitmentValidationResult {
    bool valid = false;
    std::string reason;
};

CommitmentValidationResult ValidatePayloadEncoding(const Commitment& commitment);

uint64_t SignedStakeWeight(const Commitment& commitment);

CommitmentValidationResult ValidateFinalityQuorum(const Commitment& commitment,
                                                  uint64_t active_stake,
                                                  uint64_t minimum_numerator = 2,
                                                  uint64_t minimum_denominator = 3);

std::string SourceChainName(SourceChain source_chain);

}  // namespace pantheon::common
