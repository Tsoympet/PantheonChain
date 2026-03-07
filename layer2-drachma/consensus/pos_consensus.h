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
// Slashing constants (basis points, matching genesis_drachma.json)
//   DOUBLE_SIGN_SLASH_BIPS  = 500  → 5 %  of validator stake
//   EQUIVOCATION_SLASH_BIPS = 1000 → 10 % of validator stake
// ---------------------------------------------------------------------------
static constexpr uint64_t DOUBLE_SIGN_SLASH_BIPS  = 500;
static constexpr uint64_t EQUIVOCATION_SLASH_BIPS = 1000;

// ---------------------------------------------------------------------------
// Apply slashing to a validator for the given reason.
//
// Supported reasons: "double_sign" (5%), "equivocation" (10%).
// The validator's stake is reduced in-place by the slashed amount.
// Returns a SlashingEvent describing the penalty applied.
// Throws std::invalid_argument for an unrecognised reason string.
// ---------------------------------------------------------------------------
SlashingEvent ApplySlashing(Validator& validator, const std::string& reason);

// ---------------------------------------------------------------------------
// Validate an L3 (OBOLOS) commitment that is being anchored into DRACHMA.
// `active_stake` is the total stake weight of signing validators.
// ---------------------------------------------------------------------------
common::CommitmentValidationResult ValidateL3Commit(const common::Commitment& commitment,
                                                    uint64_t last_l3_height,
                                                    uint64_t active_stake);
