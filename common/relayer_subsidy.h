// PantheonChain — Relayer Subsidy
//
// DRACHMA validators that submit TX_L2_COMMIT to TALANTON must pay TALN fees
// from their own balance.  OBOLOS validators that submit TX_L3_COMMIT to
// DRACHMA must pay DRM fees.  Without reimbursement, relayers have a net loss
// on every checkpoint submission, creating an economic incentive to stop
// relaying and breaking the settlement hierarchy.
//
// This header defines the on-chain subsidy that is credited to the relayer
// address each time an accepted commitment is applied:
//
//   TX_L2_COMMIT accepted on TALANTON → DRACHMA subsidy paid to the submitter
//     (funded from the DRM block-subsidy pool of the DRACHMA chain)
//   TX_L3_COMMIT accepted on DRACHMA  → OBOLOS subsidy paid to the submitter
//     (funded from the OBL block-subsidy pool of the OBOLOS chain)
//
// The subsidy is a fixed amount per accepted checkpoint, deducted from the
// block reward of the DRACHMA / OBOLOS proposer that includes the acceptance.
// This keeps the incentive simple, deterministic, and consensus-enforced.

#pragma once

#include <cstdint>
#include <string>

namespace pantheon::common {

// ---------------------------------------------------------------------------
// Denomination constant (mirrors asset.h / units.h)
// ---------------------------------------------------------------------------
static constexpr uint64_t RELAYER_SUBSIDY_BASE_UNIT = 100000000ULL;  // 1 token = 1e8 base units

// ---------------------------------------------------------------------------
// Subsidy constants (in base units; 1 base unit = 10^-8 of the native token)
// ---------------------------------------------------------------------------

// 1 DRM per accepted TX_L2_COMMIT — paid to the DRACHMA validator that submits
// the commitment to TALANTON.  Funded by a deduction from the DRM block reward
// of the epoch in which the commitment is accepted.
static constexpr uint64_t CHECKPOINT_SUBSIDY_DRM_BASE_UNITS = 1ULL * RELAYER_SUBSIDY_BASE_UNIT;  // 1 DRM

// 1 OBL per accepted TX_L3_COMMIT — paid to the OBOLOS validator that submits
// the commitment to DRACHMA.  Funded by a deduction from the OBL block reward
// of the epoch in which the commitment is accepted.
static constexpr uint64_t CHECKPOINT_SUBSIDY_OBL_BASE_UNITS = 1ULL * RELAYER_SUBSIDY_BASE_UNIT;  // 1 OBL

// ---------------------------------------------------------------------------
// Which layer the relayer is operating on
// ---------------------------------------------------------------------------
enum class RelayerLayer {
    L2_TO_L1,  // DRACHMA validator relaying to TALANTON (subsidy in DRM)
    L3_TO_L2,  // OBOLOS validator relaying to DRACHMA  (subsidy in OBL)
};

// ---------------------------------------------------------------------------
// RelayerSubsidy — describes the subsidy awarded to a relayer.
// ---------------------------------------------------------------------------
struct RelayerSubsidy {
    RelayerLayer layer;
    std::string  relayer_id;          // validator / relayer address
    uint64_t     amount_base_units;   // subsidy amount in native-token base units
    std::string  token_ticker;        // "DRM" or "OBL"
};

// ---------------------------------------------------------------------------
// CalculateRelayerSubsidy
//
// Returns the RelayerSubsidy that must be credited to `relayer_id` when the
// accepted commitment is included in a block.  The caller (block-reward
// application logic) is responsible for deducting this amount from the block
// proposer's reward before crediting the proposer, and crediting the returned
// amount to `relayer_id`.
//
// If the commitment_interval is zero the function returns a zero-amount
// subsidy (no-op) so callers do not need to special-case genesis.
// ---------------------------------------------------------------------------
inline RelayerSubsidy CalculateRelayerSubsidy(RelayerLayer layer,
                                              const std::string& relayer_id,
                                              uint64_t commitment_epoch) {
    (void)commitment_epoch;  // reserved for future epoch-scaled subsidy

    switch (layer) {
        case RelayerLayer::L2_TO_L1:
            return {layer, relayer_id, CHECKPOINT_SUBSIDY_DRM_BASE_UNITS, "DRM"};
        case RelayerLayer::L3_TO_L2:
            return {layer, relayer_id, CHECKPOINT_SUBSIDY_OBL_BASE_UNITS, "OBL"};
    }
    // unreachable, but keep compilers happy
    return {layer, relayer_id, 0, ""};
}

}  // namespace pantheon::common
