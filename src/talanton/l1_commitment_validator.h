#pragma once

#include "common/commitments.h"

#include <cstdint>

namespace pantheon::talanton {

struct L2AnchorState {
    uint64_t last_finalized_height = 0;
};

common::CommitmentValidationResult ValidateL2Commit(const common::Commitment& commitment,
                                                    const L2AnchorState& anchor_state,
                                                    uint64_t active_stake);

}  // namespace pantheon::talanton
