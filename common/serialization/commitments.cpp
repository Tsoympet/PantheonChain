#include "commitments.h"

#include <algorithm>
#include <unordered_set>

namespace pantheon::common {

namespace {

bool IsHexLike(const std::string& input) {
    return !input.empty() && std::all_of(input.begin(), input.end(), [](unsigned char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    });
}

}  // namespace

CommitmentValidationResult ValidatePayloadEncoding(const Commitment& commitment) {
    if (commitment.finalized_height == 0) {
        return {false, "finalized_height must be non-zero"};
    }
    if (!IsHexLike(commitment.finalized_block_hash) || commitment.finalized_block_hash.size() != 64) {
        return {false, "finalized_block_hash must be a 32-byte hex string"};
    }
    if (!IsHexLike(commitment.state_root) || commitment.state_root.size() != 64) {
        return {false, "state_root must be a 32-byte hex string"};
    }
    if (!IsHexLike(commitment.validator_set_hash) || commitment.validator_set_hash.size() != 64) {
        return {false, "validator_set_hash must be a 32-byte hex string"};
    }
    if (commitment.signatures.empty()) {
        return {false, "at least one validator signature is required"};
    }

    if (commitment.source_chain == SourceChain::DRACHMA && commitment.upstream_commitment_hash.empty()) {
        return {false, "DRACHMA commitments must include upstream OBOLOS commitment hash"};
    }
    if (!commitment.upstream_commitment_hash.empty() &&
        (!IsHexLike(commitment.upstream_commitment_hash) || commitment.upstream_commitment_hash.size() != 64)) {
        return {false, "upstream_commitment_hash must be a 32-byte hex string when present"};
    }

    for (const auto& sig : commitment.signatures) {
        if (sig.validator_id.empty() || sig.signature.empty()) {
            return {false, "validator signatures must contain validator_id and signature"};
        }
    }

    return {true, ""};
}

uint64_t SignedStakeWeight(const Commitment& commitment) {
    uint64_t total = 0;
    std::unordered_set<std::string> seen;
    for (const auto& sig : commitment.signatures) {
        if (seen.insert(sig.validator_id).second) {
            total += sig.stake_weight;
        }
    }
    return total;
}

CommitmentValidationResult ValidateFinalityQuorum(const Commitment& commitment,
                                                  uint64_t active_stake,
                                                  uint64_t minimum_numerator,
                                                  uint64_t minimum_denominator) {
    if (minimum_denominator == 0 || minimum_numerator > minimum_denominator) {
        return {false, "invalid quorum threshold"};
    }
    if (active_stake == 0) {
        return {false, "active stake cannot be zero"};
    }

    const uint64_t signed_weight = SignedStakeWeight(commitment);
    // Guard cross-multiplication against uint64_t overflow.
    // signed_weight / active_stake >= minimum_numerator / minimum_denominator
    // ↔ signed_weight * minimum_denominator >= active_stake * minimum_numerator
    const uint64_t lhs = (minimum_denominator == 0 ||
                          signed_weight <= UINT64_MAX / minimum_denominator)
                             ? signed_weight * minimum_denominator
                             : UINT64_MAX;
    const uint64_t rhs = (minimum_numerator == 0 ||
                          active_stake <= UINT64_MAX / minimum_numerator)
                             ? active_stake * minimum_numerator
                             : UINT64_MAX;
    if (lhs < rhs) {
        return {false, "finality quorum not reached"};
    }
    return {true, ""};
}

std::string SourceChainName(SourceChain source_chain) {
    switch (source_chain) {
        case SourceChain::DRACHMA:
            return "DRACHMA";
        case SourceChain::OBOLOS:
            return "OBOLOS";
        default:
            return "UNKNOWN";
    }
}

}  // namespace pantheon::common
