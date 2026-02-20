#include "l1_commitment_validator.h"

namespace pantheon::talanton {

common::CommitmentValidationResult ValidateL2Commit(const common::Commitment& commitment,
                                                    const L2AnchorState& anchor_state,
                                                    uint64_t active_stake) {
    if (commitment.source_chain != common::SourceChain::DRACHMA) {
        return {false, "TX_L2_COMMIT must originate from DRACHMA"};
    }

    if (commitment.finalized_height <= anchor_state.last_finalized_height) {
        return {false, "TX_L2_COMMIT finalized_height must be monotonic"};
    }

    auto encoding = common::ValidatePayloadEncoding(commitment);
    if (!encoding.valid) {
        return encoding;
    }

    return common::ValidateFinalityQuorum(commitment, active_stake);
}

}  // namespace pantheon::talanton
