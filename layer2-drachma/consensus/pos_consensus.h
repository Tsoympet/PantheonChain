// PantheonChain — Layer-2 DRACHMA PoS/BFT Consensus
// Block producers are validators operating under stake-weighted BFT consensus.

#pragma once

#include "common/commitments.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::drachma {

// ---------------------------------------------------------------------------
// Validator — a PoS/BFT block producer identified by its ID and stake weight.
// ---------------------------------------------------------------------------
struct Validator {
    std::string id;
    uint64_t stake = 0;  // stake weight in consensus units
};

// ---------------------------------------------------------------------------
// SlashingEvent — on-chain slashing record for equivocation / misbehaviour.
// ---------------------------------------------------------------------------
struct SlashingEvent {
    std::string validator_id;
    std::string reason;
    uint64_t    slashed_amount = 0;
};

// ---------------------------------------------------------------------------
// Compute the total active stake of all validators.
// ---------------------------------------------------------------------------
uint64_t TotalActiveStake(const std::vector<Validator>& validators);

// ---------------------------------------------------------------------------
// Select the block proposer deterministically from the active validator set
// using a stake-weighted slot assignment:
//   slot = (epoch << 32) XOR height
//   cursor = slot % total_stake
// The validator whose cumulative range covers `cursor` is selected.
// ---------------------------------------------------------------------------
const Validator& SelectDeterministicProposer(const std::vector<Validator>& validators,
                                              uint64_t epoch,
                                              uint64_t height);

// ---------------------------------------------------------------------------
// Validate an L3 (OBOLOS) commitment that is being anchored into DRACHMA.
// `active_stake` is the total stake weight of signing validators.
// ---------------------------------------------------------------------------
common::CommitmentValidationResult ValidateL3Commit(const common::Commitment& commitment,
                                                    uint64_t last_l3_height,
                                                    uint64_t active_stake);

}  // namespace pantheon::drachma
