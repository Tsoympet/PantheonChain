#include "serialization.h"

#include <sstream>

namespace pantheon::common {

std::string EncodeCommitment(const Commitment& commitment) {
    std::ostringstream stream;
    stream << SourceChainName(commitment.source_chain) << ':' << commitment.epoch << ':'
           << commitment.finalized_height << ':' << commitment.finalized_block_hash << ':'
           << commitment.state_root << ':' << commitment.validator_set_hash;
    return stream.str();
}

}  // namespace pantheon::common
