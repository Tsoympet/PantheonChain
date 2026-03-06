// PantheonChain — Layer-3 OBOLOS PoW Consensus
// Replaces the former PoS/BFT validator model.
// Block producers are PoW miners; proposer selection uses accumulated
// hash power rather than staked tokens.

#pragma once

#include "common/commitments.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pantheon::obolos {

// ---------------------------------------------------------------------------
// Miner — a PoW block producer.
// ---------------------------------------------------------------------------
struct Miner {
    std::string id;
    uint64_t hash_power = 0;  // accumulated PoW difficulty contribution
};

// Legacy type alias for source compatibility.
using Validator = Miner;

// ---------------------------------------------------------------------------
// Compute the total hash power of all active miners.
// ---------------------------------------------------------------------------
uint64_t TotalHashPower(const std::vector<Miner>& miners);

// Legacy name kept for source compatibility.
inline uint64_t TotalActiveStake(const std::vector<Miner>& miners) {
    return TotalHashPower(miners);
}

// ---------------------------------------------------------------------------
// Select the block proposer deterministically from the active miner set.
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
// Build an L3 commitment payload for anchoring to DRACHMA.
// ---------------------------------------------------------------------------
common::Commitment BuildL3Commitment(uint64_t epoch,
                                     uint64_t finalized_height,
                                     const std::string& finalized_block_hash,
                                     const std::string& state_root,
                                     const std::string& validator_set_hash,
                                     std::vector<common::FinalitySignature> signatures);

// ---------------------------------------------------------------------------
// Validate L3 finality: >=2/3 of contributing hash power must have signed.
// `active_pow` replaces the former `active_stake` parameter.
// ---------------------------------------------------------------------------
common::CommitmentValidationResult ValidateL3Finality(const common::Commitment& commitment,
                                                      uint64_t last_finalized_height,
                                                      uint64_t active_pow);

}  // namespace pantheon::obolos
