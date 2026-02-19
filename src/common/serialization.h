#pragma once

#include "commitments.h"

#include <string>

namespace pantheon::common {

std::string EncodeCommitment(const Commitment& commitment);

CommitmentValidationResult DecodeCommitment(const std::string& encoded,
                                            Commitment& commitment);

}  // namespace pantheon::common
