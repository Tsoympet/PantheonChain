#pragma once

#include "commitments.h"

#include <string>

namespace pantheon::common {

std::string EncodeCommitment(const Commitment& commitment);

}  // namespace pantheon::common
