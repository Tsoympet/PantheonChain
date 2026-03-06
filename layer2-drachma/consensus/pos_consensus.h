// PantheonChain — Layer-2 DRACHMA PoW Consensus
// Replaces the former PoS/BFT validator model.
// Block producers are PoW miners; proposer selection uses accumulated
// hash power rather than staked tokens.

#pragma once

#include "common/commitments.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::drachma {

// ---------------------------------------------------------------------------
// Miner — a PoW block producer identified by its ID and hash-power
// contribution (measured in arbitrary difficulty units, e.g. total work
// submitted since epoch start).
// ---------------------------------------------------------------------------
struct Miner {
    std::string id;
    uint64_t hash_power = 0;  // accumulated PoW difficulty contribution
};

// ---------------------------------------------------------------------------
// Legacy type alias kept for source compatibility with call sites that still
// use the name "Validator".  New code should use Miner.
// ---------------------------------------------------------------------------
using Validator = Miner;

// ---------------------------------------------------------------------------
// SlashingEvent — retained for interface compatibility but unused in PoW.
// PoW miners are penalised by orphaned blocks (wasted electricity), not by
// on-chain slashing.
// ---------------------------------------------------------------------------
struct SlashingEvent {
    std::string miner_id;
    std::string reason;
    uint64_t    slashed_amount = 0;
};

// ---------------------------------------------------------------------------
// Compute the total hash power of all active miners.
// ---------------------------------------------------------------------------
uint64_t TotalHashPower(const std::vector<Miner>& miners);

// Legacy name kept for source compatibility.
inline uint64_t TotalActiveStake(const std::vector<Miner>& miners) {
    return TotalHashPower(miners);
}

// ---------------------------------------------------------------------------
// Select the block proposer deterministically from the active miner set
// using a hash-power-weighted slot assignment:
//   slot = (epoch << 32) XOR height
//   cursor = slot % total_hash_power
// The miner whose cumulative range covers `cursor` is selected.
// ---------------------------------------------------------------------------
const Miner& SelectMiner(const std::vector<Miner>& miners,
                         uint64_t epoch,
                         uint64_t height);

// Legacy name kept for source compatibility.
inline const Miner& SelectDeterministicProposer(const std::vector<Miner>& miners,
                                                uint64_t epoch,
                                                uint64_t height) {
    return SelectMiner(miners, epoch, height);
}

// ---------------------------------------------------------------------------
// Validate an L3 (OBOLOS) commitment that is being anchored into DRACHMA.
// In PoW mode the `active_pow` parameter represents the total hash power
// of signing miners (replaces `active_stake` from the PoS era).
// ---------------------------------------------------------------------------
common::CommitmentValidationResult ValidateL3Commit(const common::Commitment& commitment,
                                                    uint64_t last_l3_height,
                                                    uint64_t active_pow);

}  // namespace pantheon::drachma
