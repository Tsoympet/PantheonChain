// PantheonChain — Layer-3 OBOLOS PoS/BFT Consensus
// Block producers are validators operating under stake-weighted BFT consensus.

#pragma once

#include "common/commitments.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::obolos {

// ---------------------------------------------------------------------------
// Validator — a PoS/BFT block producer identified by its ID and stake weight.
// ---------------------------------------------------------------------------
struct Validator {
    std::string id;
    uint64_t stake = 0;  // stake weight in consensus units
};

// ---------------------------------------------------------------------------
// Deprecated: legacy PoW-era type alias.  Use Validator instead.
// ---------------------------------------------------------------------------
using Miner [[deprecated("Use Validator (PoW-era alias; will be removed in a future release)")]] = Validator;

// ---------------------------------------------------------------------------
// Compute the total active stake of all validators.
// ---------------------------------------------------------------------------
uint64_t TotalActiveStake(const std::vector<Validator>& validators);

// ---------------------------------------------------------------------------
// Deprecated: legacy PoW-era name.  Use TotalActiveStake instead.
// ---------------------------------------------------------------------------
[[deprecated("Use TotalActiveStake (PoW-era alias; will be removed in a future release)")]]
inline uint64_t TotalHashPower(const std::vector<Validator>& validators) {
    return TotalActiveStake(validators);
}

// ---------------------------------------------------------------------------
// Select the block proposer deterministically from the active validator set.
// ---------------------------------------------------------------------------
const Validator& SelectDeterministicProposer(const std::vector<Validator>& validators,
                                              uint64_t epoch,
                                              uint64_t height);

// ---------------------------------------------------------------------------
// Deprecated: legacy PoW-era name.  Use SelectDeterministicProposer instead.
// ---------------------------------------------------------------------------
[[deprecated("Use SelectDeterministicProposer (PoW-era alias; will be removed in a future release)")]]
inline const Validator& SelectMiner(const std::vector<Validator>& validators,
                                    uint64_t epoch,
                                    uint64_t height) {
    return SelectDeterministicProposer(validators, epoch, height);
}

// ---------------------------------------------------------------------------
// Build an L3 commitment payload for anchoring to DRACHMA.
// ---------------------------------------------------------------------------
common::Commitment BuildL3Commitment(uint64_t epoch,
                                     uint64_t finalized_height,
                                     const std::string& finalized_block_hash,
                                     const std::string& state_root,
                                     const std::string& validator_set_hash,
                                     std::vector<common::FinalitySignature> signatures);

// ---------------------------------------------------------------------------
// Validate L3 finality: >=2/3 of contributing stake must have signed.
// ---------------------------------------------------------------------------
common::CommitmentValidationResult ValidateL3Finality(const common::Commitment& commitment,
                                                      uint64_t last_finalized_height,
                                                      uint64_t active_stake);

}  // namespace pantheon::obolos
